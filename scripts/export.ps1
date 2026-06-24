param(
    [string]$Preset = "release",
    [string]$AssetRoot = "assets",
    [string]$OutputDir = "build/export"
)

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmake) {
    $cmakeExe = $cmake.Source
} elseif (Test-Path "C:\Program Files\CMake\bin\cmake.exe") {
    $cmakeExe = "C:\Program Files\CMake\bin\cmake.exe"
} else {
    throw "CMake was not found on PATH or at C:\Program Files\CMake\bin\cmake.exe"
}

& $cmakeExe --preset $Preset
& $cmakeExe --build --preset $Preset --target FaahhderGame FaahhderPacker

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$packer = "build/$Preset/tools/FaahhderPacker/Release/FaahhderPacker.exe"
if (-not (Test-Path $packer)) {
    $packer = "build/$Preset/tools/FaahhderPacker/FaahhderPacker.exe"
}
if (-not (Test-Path $packer)) {
    $packer = "build/$Preset/tools/FaahhderPacker/FaahhderPacker"
}

& $packer $AssetRoot "$OutputDir/game.faahhderpack"
