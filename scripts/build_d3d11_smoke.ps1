$ErrorActionPreference = 'Stop'
Set-Location (Split-Path -Parent $PSScriptRoot)
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw "cl.exe não encontrado. Abra o Developer PowerShell for Visual Studio com Desktop development with C++ e Windows SDK instalados."
}
if (-not (Test-Path "platform\windows\d3d11_triangle.cpp")) {
    throw "Fonte do teste não encontrada: platform\windows\d3d11_triangle.cpp"
}
New-Item -ItemType Directory -Force -Path "build" | Out-Null
$source = (Resolve-Path "platform\windows\d3d11_triangle.cpp").Path
$output = Join-Path (Resolve-Path "build").Path "d3d11_triangle.exe"
Write-Host ([Environment]::NewLine + "Compilando smoke test isolado D3D11...")
cl.exe /nologo /std:c++17 /EHsc /W4 /permissive- /DUNICODE /D_UNICODE $source "/Fe:$output" /link d3d11.lib dxgi.lib d3dcompiler.lib user32.lib
if ($LASTEXITCODE -ne 0) {
    throw "Compilação falhou (exit code $LASTEXITCODE). Guarde a saída completa; não declare o teste aprovado."
}
Write-Host ([Environment]::NewLine + "Build PASS: $output")
Write-Host "Execute com: .\build\d3d11_triangle.exe"
