param(
    [string]$SourceRoot = "Source",
    [string]$OutputExe = "Distribution/alpha/Miss_qing/Pet.exe",
    [string]$ObjectDir = "Build/obj"
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projectRoot

$sourcePath = Join-Path $projectRoot $SourceRoot
if (-not (Test-Path -LiteralPath $sourcePath)) {
    throw "Source root not found: $SourceRoot"
}

if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    throw "cl.exe not found. Please run this script in Developer PowerShell for Visual Studio, or call vcvars64.bat first."
}

$cppFiles = Get-ChildItem -LiteralPath $sourcePath -Recurse -File -Filter "*.cpp" |
    Sort-Object FullName |
    ForEach-Object { $_.FullName }

if ($cppFiles.Count -eq 0) {
    throw "No .cpp files found under $SourceRoot"
}

$outputPath = Join-Path $projectRoot $OutputExe
$outputDir = Split-Path -Parent $outputPath
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

$objectPath = Join-Path $projectRoot $ObjectDir
New-Item -ItemType Directory -Force -Path $objectPath | Out-Null

$compileArgs = @(
    "/std:c++17",
    "/EHsc",
    "/utf-8",
    "/I", $sourcePath,
    "/Fo$objectPath\\"
) + $cppFiles + @(
    "/Fe:$outputPath",
    "/link",
    "user32.lib",
    "gdi32.lib",
    "gdiplus.lib",
    "winmm.lib",
    "shell32.lib",
    "imm32.lib"
)

Write-Host "Compiling $($cppFiles.Count) source files from '$SourceRoot'..."
& cl @compileArgs

if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}

Write-Host "Build succeeded: $OutputExe"
