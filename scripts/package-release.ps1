param(
    [string]$Version = "v0.1.0",
    [string]$BuildDir = "build\Release",
    [string]$ModelPath = "models\ggml-small.bin",
    [string]$InnoSetupPath = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $repoRoot $BuildDir
$modelFile = Join-Path $repoRoot $ModelPath
$distPath = Join-Path $repoRoot "dist"
$stagePath = Join-Path $distPath "FIR-$Version"
$installerScript = Join-Path $repoRoot "installer\FIR.iss"

foreach ($requiredPath in @(
    (Join-Path $buildPath "RadioAccessTFG.exe"),
    (Join-Path $repoRoot "web"),
    (Join-Path $repoRoot "tools\nginx\nginx.exe"),
    (Join-Path $repoRoot "LICENSE"),
    (Join-Path $repoRoot "THIRD_PARTY_NOTICES.md"),
    $modelFile,
    $installerScript
)) {
    if (-not (Test-Path $requiredPath)) {
        throw "Missing release input: $requiredPath"
    }
}

$runtimeDlls = @(Get-ChildItem $buildPath -Filter "*.dll" -File)
if ($runtimeDlls.Count -eq 0) {
    throw "No runtime DLLs were found in: $buildPath"
}

Remove-Item $stagePath -Recurse -Force -ErrorAction SilentlyContinue
New-Item $stagePath -ItemType Directory -Force | Out-Null
New-Item (Join-Path $stagePath "models") -ItemType Directory -Force | Out-Null

Copy-Item (Join-Path $buildPath "RadioAccessTFG.exe") $stagePath
$runtimeDlls | Copy-Item -Destination $stagePath
Copy-Item $modelFile (Join-Path $stagePath "models\ggml-small.bin")
Copy-Item (Join-Path $repoRoot "web") $stagePath -Recurse
New-Item (Join-Path $stagePath "tools") -ItemType Directory -Force | Out-Null
Copy-Item (Join-Path $repoRoot "tools\nginx") (Join-Path $stagePath "tools") -Recurse
Remove-Item (Join-Path $stagePath "tools\nginx\logs") -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $stagePath "tools\nginx\temp") -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item (Join-Path $repoRoot "LICENSE") $stagePath
Copy-Item (Join-Path $repoRoot "THIRD_PARTY_NOTICES.md") $stagePath
Copy-Item (Join-Path $repoRoot "README.md") $stagePath
@"
# FIR configuration
QRZ_USER=
QRZ_PASS=
MY_CALLSIGN=
"@ | Set-Content (Join-Path $stagePath ".env") -Encoding ascii

$portablePath = Join-Path $distPath "FIR-$Version-portable.zip"
Remove-Item $portablePath -Force -ErrorAction SilentlyContinue
Compress-Archive -Path $stagePath -DestinationPath $portablePath -CompressionLevel Optimal

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::Open($portablePath, [System.IO.Compression.ZipArchiveMode]::Update)
try {
    $normalizedEnvPath = "FIR-$Version/.env"
    $existingEnv = $archive.Entries | Where-Object {
        ($_.FullName -replace '\\', '/') -eq $normalizedEnvPath
    } | Select-Object -First 1
    if (-not $existingEnv) {
        $envEntry = $archive.CreateEntry($normalizedEnvPath)
        $sourceEnv = [System.IO.File]::ReadAllBytes((Join-Path $stagePath ".env"))
        $stream = $envEntry.Open()
        try {
            $stream.Write($sourceEnv, 0, $sourceEnv.Length)
        }
        finally {
            $stream.Dispose()
        }
    }
}
finally {
    $archive.Dispose()
}

if (-not $InnoSetupPath) {
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) {
        $InnoSetupPath = $command.Source
    }
}

if (-not $InnoSetupPath) {
    $candidatePaths = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe"
    )
    $InnoSetupPath = $candidatePaths | Where-Object { Test-Path $_ } | Select-Object -First 1
}

if (-not $InnoSetupPath) {
    throw "Inno Setup 6 was not found. Install it or pass -InnoSetupPath to create the installer."
}

& $InnoSetupPath "/DMyAppVersion=$Version" "/DSourceDir=$stagePath" "/DOutputDir=$distPath" $installerScript
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup failed with exit code $LASTEXITCODE."
}

Write-Host "Created: $portablePath"
Write-Host "Created installer in: $distPath"