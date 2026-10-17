$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw "Abra o Developer PowerShell do Visual Studio com C++ e Windows SDK." }
$files = @("platform\windows\d3d11_bmd_preview.cpp","src\mu\assets\bmd.cpp","src\mu\assets\bmd_body.cpp","src\mu\assets\mesh.cpp","src\mu\assets\format.cpp")
foreach($f in $files){if(-not(Test-Path $f)){throw "Fonte ausente: $f"}}
New-Item -ItemType Directory -Force -Path build | Out-Null
$sources = $files | ForEach-Object { (Resolve-Path $_).Path }
$out = Join-Path (Resolve-Path build).Path "d3d11_bmd_preview.exe"
& cl.exe /nologo /std:c++17 /EHsc /W4 /permissive- /DUNICODE /D_UNICODE /I src @sources "/Fe:$out" /link d3d11.lib dxgi.lib d3dcompiler.lib user32.lib
if($LASTEXITCODE -ne 0){throw "Build falhou com exit code $LASTEXITCODE."}
Write-Host "Build criado: $out"
Write-Host 'Uso: .\build\d3d11_bmd_preview.exe "C:\caminho\modelo.bmd"'
Write-Host "Somente BMD v0x0A em texto claro; sem texturas, animação ou skinning."
