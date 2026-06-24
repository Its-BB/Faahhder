param(
    [string]$Project = "examples/snake",
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
& $cmakeExe --build --preset $Preset --target FaahhderGame

$projectPath = Resolve-Path $Project
$meta = @{}
Get-Content (Join-Path $projectPath "project.faahhder") | ForEach-Object {
    $parts = $_ -split "=", 2
    if ($parts.Count -eq 2) {
        $meta[$parts[0].Trim()] = $parts[1].Trim()
    }
}

$name = if ($meta.ContainsKey("name")) { $meta["name"] } else { Split-Path $projectPath -Leaf }
$safeName = ($name -replace '[^A-Za-z0-9_-]', '_')
$outDir = Join-Path "dist" $safeName
if (Test-Path $outDir) {
    Remove-Item -LiteralPath $outDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$runner = "build/$Preset/src/FaahhderGame/Debug/FaahhderGame.exe"
if (-not (Test-Path $runner)) {
    $runner = "build/$Preset/src/FaahhderGame/Release/FaahhderGame.exe"
}
if (-not (Test-Path $runner)) {
    throw "FaahhderGame.exe was not found after build."
}

Copy-Item -LiteralPath $runner -Destination (Join-Path $outDir "$safeName.exe") -Force
Copy-Item -LiteralPath (Join-Path $projectPath "project.faahhder") -Destination (Join-Path $outDir "project.faahhder") -Force
Copy-Item -LiteralPath (Join-Path $projectPath "assets") -Destination $outDir -Recurse -Force
Write-Output "Built $outDir"

