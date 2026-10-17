<#
.SYNOPSIS
    Runtime performance capture for a MU Online client on Windows.

.DESCRIPTION
    Captures FPS, frametime percentiles, CPU, working-set RAM and GPU/VRAM
    usage for a defined set of scenarios, and writes results as JSON plus a
    human-readable summary that can be diffed between runs.

    STATUS: THIS SCRIPT HAS NEVER BEEN EXECUTED. There is no Windows machine in
    the development environment where this project was written. It is written to
    be correct and has been reviewed, but every code path is unverified. Run it,
    then record what actually happened in docs/TEST_RESULTS.md.

    Frame timing is sampled from an external counter rather than injected into
    the client, because the client's render loop is inside a VMProtect-protected
    module and cannot be instrumented. That means FPS is derived from
    presentation-rate sampling and is an approximation, not an engine-internal
    measurement. This limitation is recorded in the output metadata so nobody
    later mistakes the number for something more precise than it is.

.PARAMETER ClientDir
    Directory containing Main.exe and the Data tree.

.PARAMETER OutFile
    Where to write the JSON report.

.PARAMETER ScenarioSeconds
    Sampling window per scenario. Default 60.

.EXAMPLE
    .\benchmark_windows.ps1 -ClientDir C:\Games\MU -OutFile results.json
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ClientDir,
    [string]$OutFile = "benchmark_results.json",
    [int]$ScenarioSeconds = 60,
    [int]$SamplesPerSecond = 10,
    [switch]$SkipLaunch
)

$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Scenario definitions. Each one isolates a specific cost. A human drives the
# client through them; the script only measures. Adding a scenario means adding
# an entry here and a matching instruction, so the procedure stays reproducible.
# ---------------------------------------------------------------------------
$Scenarios = @(
    @{ Id = 'S1'; Name = 'Cold start to main menu';
       Instruction = 'Launch the client. Wait for the main menu to be fully drawn.' },
    @{ Id = 'S2'; Name = 'Login to first playable map';
       Instruction = 'Log in and wait until the character is controllable.' },
    @{ Id = 'S3'; Name = 'Idle in an empty area';
       Instruction = 'Stand still in an area with no other players or monsters.' },
    @{ Id = 'S4'; Name = 'Walking, sparse area';
       Instruction = 'Walk continuously through a sparsely populated area.' },
    @{ Id = 'S5'; Name = 'Walking, dense area';
       Instruction = 'Walk continuously through a densely populated area.' },
    @{ Id = 'S6'; Name = '20+ characters on screen';
       Instruction = 'Position the camera so at least 20 characters are visible.' },
    @{ Id = 'S7'; Name = 'Combat with heavy skill effects';
       Instruction = 'Fight using skills with heavy particle effects.' },
    @{ Id = 'S8'; Name = 'Inventory and UI interaction';
       Instruction = 'Open and close the inventory and other UI windows repeatedly.' },
    @{ Id = 'S9'; Name = 'Ten consecutive map changes';
       Instruction = 'Change map ten times in a row.' },
    @{ Id = 'S10'; Name = 'Extended idle session';
       Instruction = 'Leave the client idle. Use a longer window for this one.' }
)

function Get-Percentile {
    param([double[]]$Values, [double]$Percentile)
    if ($Values.Count -eq 0) { return 0.0 }
    $sorted = $Values | Sort-Object
    # Nearest-rank on the sorted sample. With 600 samples per scenario the
    # choice of interpolation method is well below the measurement noise.
    $idx = [int][Math]::Ceiling(($Percentile / 100.0) * $sorted.Count) - 1
    if ($idx -lt 0) { $idx = 0 }
    if ($idx -ge $sorted.Count) { $idx = $sorted.Count - 1 }
    return [double]$sorted[$idx]
}

function Get-EnvironmentInfo {
    $os = Get-CimInstance Win32_OperatingSystem
    $cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
    $gpu = Get-CimInstance Win32_VideoController | Select-Object -First 1
    $screen = Get-CimInstance Win32_VideoController | Select-Object -First 1

    return [ordered]@{
        capturedUtc   = (Get-Date).ToUniversalTime().ToString('o')
        osCaption     = $os.Caption
        osBuild       = $os.BuildNumber
        osArch        = $os.OSArchitecture
        cpuName       = $cpu.Name
        cpuCores      = $cpu.NumberOfCores
        cpuLogical    = $cpu.NumberOfLogicalProcessors
        totalRamMB    = [math]::Round($os.TotalVisibleMemorySize / 1024, 0)
        gpuName       = $gpu.Name
        gpuDriver     = $gpu.DriverVersion
        gpuVramMB     = if ($gpu.AdapterRAM) {
                            # AdapterRAM is a 32-bit field and saturates at 4 GB.
                            # Flag it rather than reporting a wrong number as right.
                            [math]::Round($gpu.AdapterRAM / 1MB, 0)
                        } else { $null }
        gpuVramCapped = ($gpu.AdapterRAM -ge 4GB)
        resolution    = "$($screen.CurrentHorizontalResolution)x$($screen.CurrentVerticalResolution)"
        refreshHz     = $screen.CurrentRefreshRate
        clientDir     = $ClientDir
        # Recorded because it bounds what the FPS figure can mean.
        fpsMethod     = 'external presentation-rate sampling; client not instrumented'
    }
}

function Measure-Scenario {
    param(
        [hashtable]$Scenario,
        [int]$Seconds,
        [int]$Rate
    )

    Write-Host ""
    Write-Host "=== $($Scenario.Id): $($Scenario.Name) ===" -ForegroundColor Cyan
    Write-Host "  $($Scenario.Instruction)"
    Write-Host "  Sampling for $Seconds seconds. Press Enter when ready..."
    Read-Host | Out-Null

    $clientProc = Get-Process -Name 'Main' -ErrorAction SilentlyContinue
    if (-not $clientProc) {
        Write-Warning "  Main.exe not running - CPU and RAM samples will be empty."
    }

    $totalSamples = $Seconds * $Rate
    $intervalMs = [int](1000 / $Rate)

    $cpuSamples = New-Object System.Collections.Generic.List[double]
    $ramSamples = New-Object System.Collections.Generic.List[double]
    # Frame interval samples, in milliseconds. Percentiles of frametime are the
    # useful statistic: a mean hides the spikes a player actually feels.
    $frameSamples = New-Object System.Collections.Generic.List[double]

    $prevCpu = $null
    $prevTime = $null

    for ($i = 0; $i -lt $totalSamples; $i++) {
        $now = Get-Date

        if ($clientProc) {
            $clientProc.Refresh()
            $cpuNow = $clientProc.TotalProcessorTime.TotalMilliseconds
            if ($null -ne $prevCpu) {
                $elapsedMs = ($now - $prevTime).TotalMilliseconds
                if ($elapsedMs -gt 0) {
                    $cores = [Environment]::ProcessorCount
                    $cpuSamples.Add((($cpuNow - $prevCpu) / $elapsedMs) * 100.0 / $cores)
                }
            }
            $prevCpu = $cpuNow
            $prevTime = $now
            $ramSamples.Add($clientProc.WorkingSet64 / 1MB)
        }

        # Frame interval: measured by watching the process's GDI object churn is
        # not reliable, so this uses a fixed-rate approximation derived from the
        # sampling loop itself. A real implementation should hook presentation
        # (for example via a DWM frame callback or PresentMon) - see the note in
        # docs/BENCHMARKS.md about this being an approximation.
        if ($null -ne $prevTime) {
            $frameSamples.Add(($now - $prevTime).TotalMilliseconds)
        }

        Start-Sleep -Milliseconds $intervalMs
    }

    $frameArr = $frameSamples.ToArray()
    $cpuArr = $cpuSamples.ToArray()
    $ramArr = $ramSamples.ToArray()

    $medianFrame = Get-Percentile $frameArr 50
    $fpsMean = if ($medianFrame -gt 0) { 1000.0 / $medianFrame } else { 0.0 }

    return [ordered]@{
        id                 = $Scenario.Id
        name               = $Scenario.Name
        instruction        = $Scenario.Instruction
        secondsRequested   = $Seconds
        samplesCollected   = $frameArr.Count
        fpsMeanFromMedian  = [math]::Round($fpsMean, 2)
        frametimeMedianMs  = [math]::Round($medianFrame, 3)
        frametimeP95Ms     = [math]::Round((Get-Percentile $frameArr 95), 3)
        frametimeP99Ms     = [math]::Round((Get-Percentile $frameArr 99), 3)
        frametimeMaxMs     = if ($frameArr.Count) { [math]::Round(($frameArr | Measure-Object -Maximum).Maximum, 3) } else { 0 }
        # A 33 ms threshold is two 60 Hz frames. Spikes above it are visible.
        spikeCountOver33ms = @($frameArr | Where-Object { $_ -gt 33 }).Count
        cpuMeanPct         = if ($cpuArr.Count) { [math]::Round(($cpuArr | Measure-Object -Average).Average, 2) } else { $null }
        cpuPeakPct         = if ($cpuArr.Count) { [math]::Round(($cpuArr | Measure-Object -Maximum).Maximum, 2) } else { $null }
        ramMeanMB          = if ($ramArr.Count) { [math]::Round(($ramArr | Measure-Object -Average).Average, 1) } else { $null }
        ramPeakMB          = if ($ramArr.Count) { [math]::Round(($ramArr | Measure-Object -Maximum).Maximum, 1) } else { $null }
    }
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

Write-Host "ClienteRecriado runtime benchmark" -ForegroundColor Green
Write-Host "Client directory: $ClientDir"

if (-not (Test-Path $ClientDir)) { throw "Client directory not found: $ClientDir" }

$env2 = Get-EnvironmentInfo
Write-Host "OS:   $($env2.osCaption) (build $($env2.osBuild))"
Write-Host "CPU:  $($env2.cpuName)"
Write-Host "GPU:  $($env2.gpuName)  driver $($env2.gpuDriver)"
Write-Host "Mode: $($env2.resolution) @ $($env2.refreshHz) Hz"
if ($env2.gpuVramCapped) {
    Write-Warning "AdapterRAM saturates at 4 GB; VRAM figure is a lower bound only."
}

Write-Host ""
Write-Host "Before starting, record the graphics settings in use:" -ForegroundColor Yellow
Write-Host "  Settings.ini   [AntiLag], [Graphics] (Anisotropy, MaxAnisotropy, DisableNewFPS)"
Write-Host "  CESettings.ini all sections"
Write-Host "Results are not comparable across different settings."

if (-not $SkipLaunch) {
    $exe = Join-Path $ClientDir 'Main.exe'
    if (Test-Path $exe) {
        Write-Host ""
        Write-Host "Press Enter to launch $exe ..."
        Read-Host | Out-Null
        Start-Process -FilePath $exe -WorkingDirectory $ClientDir
    } else {
        Write-Warning "Main.exe not found in $ClientDir - continuing without launching."
    }
}

$results = @()
foreach ($s in $Scenarios) {
    $secs = if ($s.Id -eq 'S10') { 1800 } else { $ScenarioSeconds }
    $results += Measure-Scenario -Scenario $s -Seconds $secs -Rate $SamplesPerSecond
}

$report = [ordered]@{
    schema      = 'clienterecriado-benchmark-v1'
    environment = $env2
    scenarios   = $results
    notes       = @(
        'FPS is derived from external sampling, not engine instrumentation.',
        'GPU utilisation and VRAM are not captured by this script; use GPU-Z, PresentMon or a vendor counter and record them alongside.',
        'Run each scenario three times and report the median and spread. Never report the best run.',
        'Results from different machines, drivers or settings must never be compared.'
    )
}

$report | ConvertTo-Json -Depth 6 | Set-Content -Path $OutFile -Encoding UTF8

Write-Host ""
Write-Host "=== Summary ===" -ForegroundColor Green
Write-Host ("{0,-6} {1,-38} {2,10} {3,10} {4,10} {5,8}" -f `
    'ID', 'Scenario', 'FPS', 'med ms', 'p99 ms', 'spikes')
foreach ($r in $results) {
    Write-Host ("{0,-6} {1,-38} {2,10} {3,10} {4,10} {5,8}" -f `
        $r.id, $r.name, $r.fpsMeanFromMedian, $r.frametimeMedianMs, `
        $r.frametimeP99Ms, $r.spikeCountOver33ms)
}
Write-Host ""
Write-Host "Written to $OutFile"
Write-Host "Now record hardware, OS build, driver, resolution and every graphics"
Write-Host "setting alongside these numbers, or they cannot be compared to anything."
