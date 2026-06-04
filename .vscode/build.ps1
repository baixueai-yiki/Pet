param(
    [string]$SourceRoot = "Source",
    [string]$OutputExe = "Distribution/alpha/MissCarrot/MissCarrot.exe",
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

function Get-ObjectFilePath {
    param([string]$CppFile)
    $relative = $CppFile.Substring($sourcePath.Length).TrimStart('\','/')
    $safeName = ($relative -replace '[\\/:*?"<>|]', '_') -replace '\.cpp$', '.obj'
    return Join-Path $objectPath $safeName
}

$objectFiles = @()
Write-Host "Compiling $($cppFiles.Count) source files from '$SourceRoot'..."
foreach ($cpp in $cppFiles) {
    $obj = Get-ObjectFilePath -CppFile $cpp
    $objDir = Split-Path -Parent $obj
    New-Item -ItemType Directory -Force -Path $objDir | Out-Null

    $compileArgs = @(
        "/nologo",
        "/std:c++17",
        "/EHsc",
        "/utf-8",
        "/DUNICODE",
        "/D_UNICODE",
        "/I", $sourcePath,
        "/c",
        $cpp,
        "/Fo$obj"
    )

    & cl @compileArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed compiling $cpp with exit code $LASTEXITCODE"
    }

    $objectFiles += $obj
}

# 编译资源文件（图标）
$rcFile = Join-Path $sourcePath "Pet.rc"
if (Test-Path -LiteralPath $rcFile) {
    $resFile = Join-Path $objectPath "Pet.res"
    $rcArgs = @("/nologo", "/I", $projectRoot, "/fo", $resFile, $rcFile)
    & rc @rcArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Resource compilation failed with exit code $LASTEXITCODE"
    }
    $objectFiles += $resFile
}

$linkArgs = @(
    "/nologo",
    "/Fe:$outputPath"
) + $objectFiles + @(
    "/link",
    "/SUBSYSTEM:WINDOWS",
    "/ENTRY:wWinMainCRTStartup",
    "user32.lib",
    "gdi32.lib",
    "gdiplus.lib",
    "winmm.lib",
    "shell32.lib",
    "imm32.lib"
)

& cl @linkArgs

if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}

Write-Host "Build succeeded: $OutputExe"
