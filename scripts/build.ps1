param(
    [string]$Preset = "dev"
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
& $cmakeExe --build --preset $Preset
