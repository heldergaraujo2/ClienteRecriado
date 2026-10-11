<#
.SYNOPSIS
    Build and validation checklist for the ClienteRecriado project on Windows.

.DESCRIPTION
    Runs the steps that could not be run in the Linux development environment:
    compiling with MSVC or MinGW, running the unit tests, and validating against
    a real client asset tree. Prints a PASS/FAIL/BLOCKED line per step so the
    output can be pasted straight into docs/TEST_RESULTS.md.

    STATUS: THIS SCRIPT HAS NEVER BEEN EXECUTED. There is no Windows machine in
    the environment where this project was written. Every code path is
    unverified. This is the highest-value thing a person with a Windows machine
    can do for this project, because the target platform has never been compiled
    for and every portability assumption in the codebase is currently untested.

.PARAMETER RepoDir
    Root of the ClienteRecriado checkout.

.PARAMETER ClientDir
    Optional directory containing a legally obtained client (Main.exe + Data).
    When supplied, whole-tree asset validation runs too.

.EXAMPLE
    .\validate_windows.ps1 -RepoDir C:\dev\ClienteRecriado -ClientDir C:\Games\MU
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$RepoDir,
    [string]$ClientDir
)

$ErrorActionPreference = 'Continue'
$Results = New-Object System.Collections.Generic.List[object]

function Add-Result {
    param([string]$Name, [string]$Status, [string]$Detail)
    $Results.Add([pscustomobject]@{
            Name   = $Name
            Status = $Status
            Detail = $Detail
        })
    $colour = switch ($Status) {
        'PASS' { 'Green' }
        'FAIL' { 'Red' }
        default { 'Yellow' }
    }
    Write-Host ("[{0,-13}] {1}" -f $Status, $Name) -ForegroundColor $colour
    if ($Detail) { Write-Host "                 $Detail" }
}

function Invoke-Step {
    param([string]$Name, [scriptblock]$Body)
    Write-Host ""
    Write-Host "--- $Name ---" -ForegroundColor Cyan
    try {
        $detail = & $Body
        if ($LASTEXITCODE -ne $null -and $LASTEXITCODE -ne 0) {
            Add-Result $Name 'FAIL' "exit code $LASTEXITCODE. $detail"
            return $false
        }
        Add-Result $Name 'PASS' "$detail"
        return $true
    } catch {
        Add-Result $Name 'FAIL' $_.Exception.Message
        return $false
    }
}

Write-Host "ClienteRecriado Windows validation" -ForegroundColor Green
Write-Host "Repository: $RepoDir"
Write-Host "Started:    $((Get-Date).ToUniversalTime().ToString('o'))"

if (-not (Test-Path $RepoDir)) { throw "Repository not found: $RepoDir" }
Push-Location $RepoDir
try {

    # ---------------------------------------------------------------- environment
    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    $gpp = Get-Command g++.exe -ErrorAction SilentlyContinue
    $gcc32 = Get-Command i686-w64-mingw32-g++.exe -ErrorAction SilentlyContinue

    $os = Get-CimInstance Win32_OperatingSystem
    Write-Host "OS: $($os.Caption) build $($os.BuildNumber) $($os.OSArchitecture)"
    if ($cl) { Write-Host "MSVC: found" } else { Write-Host "MSVC: not on PATH" }
    if ($gpp) { Write-Host "g++:  found" } else { Write-Host "g++:  not on PATH" }

    if (-not $cl -and -not $gpp) {
        Add-Result 'Compiler availability' 'BLOCKED' `
            'Neither cl.exe nor g++.exe found. Open an x86 Native Tools Command Prompt for MSVC, or install MinGW.'
        Write-Host ""
        Write-Host "Cannot continue without a compiler." -ForegroundColor Red
        $Results | Format-Table -AutoSize
        return
    }
    Add-Result 'Compiler availability' 'PASS' `
        "MSVC=$(if ($cl) { 'yes' } else { 'no' }) g++=$(if ($gpp) { 'yes' } else { 'no' }) mingw32=$(if ($gcc32) { 'yes' } else { 'no' })"

    # ---------------------------------------------------------------- build dir
    $build = Join-Path $RepoDir 'build'
    if (-not (Test-Path $build)) { New-Item -ItemType Directory -Path $build | Out-Null }

    $libSources = @(
        'src\mu\assets\format.cpp', 'src\mu\assets\ozt.cpp', 'src\mu\assets\ozj.cpp',
        'src\mu\assets\tga.cpp', 'src\mu\assets\bmd.cpp', 'src\mu\assets\terrain.cpp',
        'src\mu\assets\manifest.cpp', 'src\mu\fs.cpp'
    )

    # ---------------------------------------------------------------- MSVC build
    if ($cl) {
        $built = Invoke-Step 'Build test_assets with MSVC' {
            & cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /permissive- /Isrc `
                @libSources 'tests\test_assets.cpp' /Fe:build\test_assets.exe 2>&1
            if ($LASTEXITCODE -eq 0) { 'build\test_assets.exe' } else { 'compile failed' }
        }

        if ($built) {
            Invoke-Step 'Unit tests (MSVC build)' {
                $out = & .\build\test_assets.exe 2>&1
                $out | Write-Host
                if ($LASTEXITCODE -ne 0) { return 'test failures' }
                $summary = ($out | Select-String -Pattern 'checks,' | Select-Object -First 1)
                return "$summary"
            }
        }

        Invoke-Step 'Build mutool with MSVC' {
            & cl.exe /nologo /std:c++17 /O2 /EHsc /W4 /permissive- /Isrc `
                @libSources 'tools\mutool.cpp' /Fe:build\mutool.exe 2>&1
            if ($LASTEXITCODE -eq 0) { 'build\mutool.exe' } else { 'compile failed' }
        }
    } else {
        Add-Result 'Build test_assets with MSVC' 'NOT APPLICABLE' 'cl.exe not available'
        Add-Result 'Build mutool with MSVC' 'NOT APPLICABLE' 'cl.exe not available'
    }

    # ---------------------------------------------------------------- MinGW build
    if ($gpp) {
        $builtG = Invoke-Step 'Build test_assets with g++' {
            & g++.exe -std=c++17 -O2 -Wall -Wextra -Isrc `
                @libSources 'tests/test_assets.cpp' -o build/test_assets_gcc.exe 2>&1
            if ($LASTEXITCODE -eq 0) { 'build/test_assets_gcc.exe' } else { 'compile failed' }
        }
        if ($builtG) {
            Invoke-Step 'Unit tests (g++ build)' {
                $out = & .\build\test_assets_gcc.exe 2>&1
                $out | Write-Host
                if ($LASTEXITCODE -ne 0) { return 'test failures' }
                return ($out | Select-String -Pattern 'checks,' | Select-Object -First 1).ToString()
            }
        }
    } else {
        Add-Result 'Build test_assets with g++' 'NOT APPLICABLE' 'g++.exe not available'
    }

    # ---------------------------------------------------------------- asset validation
    if ($ClientDir) {
        $dataDir = Join-Path $ClientDir 'Data'
        $mutool = Join-Path $build 'mutool.exe'

        if (-not (Test-Path $dataDir)) {
            Add-Result 'Asset tree scan' 'BLOCKED' "Data directory not found at $dataDir"
        } elseif (-not (Test-Path $mutool)) {
            Add-Result 'Asset tree scan' 'BLOCKED' 'mutool.exe was not built'
        } else {
            Invoke-Step 'Asset tree scan' {
                $sw = [System.Diagnostics.Stopwatch]::StartNew()
                $out = & $mutool scan $dataDir 2>&1
                $sw.Stop()
                $out | Write-Host
                "completed in $($sw.ElapsedMilliseconds) ms. Compare the per-format ok/fail counts against AGENTS.md; any drop is a regression."
            }

            $manifest = Join-Path $ClientDir 'ListaPatch.txt'
            if (Test-Path $manifest) {
                Invoke-Step 'Patch manifest verification' {
                    $out = & $mutool verify $manifest $ClientDir 2>&1
                    $out | Write-Host
                    # Exit code 2 means problems were found, which is a valid
                    # result rather than a tool failure. Distinguish the two.
                    if ($LASTEXITCODE -eq 0) { return 'all entries match' }
                    if ($LASTEXITCODE -eq 2) { return 'mismatches reported (see above) - this is a valid result, not a tool error' }
                    return 'tool error'
                }
            } else {
                Add-Result 'Patch manifest verification' 'NOT APPLICABLE' 'ListaPatch.txt not present'
            }
        }
    } else {
        Add-Result 'Asset tree scan' 'NOT EXECUTED' 'No -ClientDir supplied'
        Add-Result 'Patch manifest verification' 'NOT EXECUTED' 'No -ClientDir supplied'
    }

    # ---------------------------------------------------------------- runtime items
    Add-Result 'Client startup' 'NOT EXECUTED' 'Run scripts\benchmark_windows.ps1 scenario S1'
    Add-Result 'FPS and frametime' 'NOT EXECUTED' 'Run scripts\benchmark_windows.ps1'
    Add-Result 'VRAM measurement' 'NOT EXECUTED' 'Use GPU-Z or a vendor counter; this script does not capture it'
    Add-Result 'Long-session stability' 'NOT EXECUTED' 'Run scenario S10 (30 minutes)'
    Add-Result 'Network compatibility' 'BLOCKED' 'No authorised test server available'
    Add-Result 'Visual comparison vs original' 'BLOCKED' 'No renderer exists yet'

} finally {
    Pop-Location
}

# ---------------------------------------------------------------------------
# Report
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "=================== RESULTS ===================" -ForegroundColor Green
$Results | Format-Table -AutoSize

$counts = $Results | Group-Object Status | ForEach-Object { "$($_.Name)=$($_.Count)" }
Write-Host ($counts -join '  ')

$stamp = (Get-Date).ToString('yyyyMMdd-HHmmss')
$reportPath = Join-Path $RepoDir "build\windows_validation_$stamp.txt"
$Results | Format-Table -AutoSize | Out-File -FilePath $reportPath -Encoding UTF8
Write-Host ""
Write-Host "Report written to $reportPath"
Write-Host "Paste these results into docs/TEST_RESULTS.md, together with the"
Write-Host "compiler version, Windows build and any warnings the build produced."

$failed = @($Results | Where-Object { $_.Status -eq 'FAIL' })
if ($failed.Count -gt 0) { exit 1 }
exit 0
