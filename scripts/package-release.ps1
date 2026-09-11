param(
    [string]$Version = "v0.1.0",
    [string]$BuildDir = "build\Release",
    [string]$ModelPath = "models\ggml-small.bin",
    [string]$InnoSetupPath = "",
    [switch]$IncludeCuda,
    [string]$CudaBinDir = "",
    [string]$CpuBinDir = "",
    [switch]$IncludeVulkan,
    [string]$VulkanBinDir = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$buildPath = Join-Path $repoRoot $BuildDir
$modelFile = Join-Path $repoRoot $ModelPath
$distPath = Join-Path $repoRoot "dist"
$variant = if ($IncludeCuda) { "cuda" } elseif ($IncludeVulkan) { "vulkan" } else { "cpu" }
$stagePath = Join-Path $distPath "FIR-$Version-$variant"
$installerScript = Join-Path $repoRoot "installer\FIR.iss"

if ($IncludeCuda -and $IncludeVulkan) {
    throw "Choose only one accelerated backend: CUDA or Vulkan."
}

foreach ($requiredPath in @(
    (Join-Path $buildPath "FIR.exe"),
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

$runtimeDlls = @(Get-ChildItem $buildPath -Filter "*.dll" -File | Where-Object {
    $_.Name -notmatch "^(ggml.*|whisper\.dll)$"
})
if ($runtimeDlls.Count -eq 0) {
    throw "No runtime DLLs were found in: $buildPath"
}

if (-not $IncludeCuda -and -not $IncludeVulkan) {
    if (-not $CpuBinDir -or -not (Test-Path $CpuBinDir)) {
        throw "CPU packaging requires -CpuBinDir pointing to the CPU whisper.cpp Release folder."
    }
    foreach ($cpuRequiredName in @("ggml-base.dll", "ggml.dll", "ggml-cpu-x64.dll", "whisper.dll")) {
        if (-not (Test-Path (Join-Path $CpuBinDir $cpuRequiredName))) {
            throw "CPU package is incomplete; missing $(Join-Path $CpuBinDir $cpuRequiredName)"
        }
    }
    $cpuDependencies = @(
        (Get-Item (Join-Path $CpuBinDir "ggml-base.dll")),
        (Get-Item (Join-Path $CpuBinDir "ggml.dll")),
        (Get-Item (Join-Path $CpuBinDir "whisper.dll"))
    ) + @(Get-ChildItem $CpuBinDir -Filter "ggml-cpu-*.dll" -File)
    $runtimeDlls += $cpuDependencies
}

if ($IncludeVulkan) {
    if (-not $VulkanBinDir -or -not (Test-Path $VulkanBinDir)) {
        throw "Vulkan packaging requires -VulkanBinDir pointing to a Vulkan whisper.cpp Release folder."
    }
    foreach ($vulkanRequiredName in @("ggml-base.dll", "ggml.dll", "ggml-cpu.dll", "ggml-vulkan.dll", "whisper.dll")) {
        if (-not (Test-Path (Join-Path $VulkanBinDir $vulkanRequiredName))) {
            throw "Vulkan package is incomplete; missing $(Join-Path $VulkanBinDir $vulkanRequiredName)"
        }
    }
    $runtimeDlls += @(
        (Get-Item (Join-Path $VulkanBinDir "ggml-base.dll")),
        (Get-Item (Join-Path $VulkanBinDir "ggml.dll")),
        (Get-Item (Join-Path $VulkanBinDir "ggml-cpu.dll")),
        (Get-Item (Join-Path $VulkanBinDir "ggml-vulkan.dll")),
        (Get-Item (Join-Path $VulkanBinDir "whisper.dll"))
    )
}

$dumpbinCandidates = @(
    "${env:VSINSTALLDIR}VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"
)
$dumpbin = $dumpbinCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $IncludeCuda -and -not $IncludeVulkan) {
    if (-not $dumpbin) {
        throw "Visual Studio dumpbin.exe is required to validate the CPU DLL dependencies."
    }
    $ggmlDependencies = & $dumpbin /DEPENDENTS (Join-Path $CpuBinDir "ggml.dll") 2>$null
    if ($ggmlDependencies -match "ggml-cuda\.dll") {
        throw "The CPU package cannot be created: the selected ggml.dll imports ggml-cuda.dll."
    }
}

if ($IncludeCuda) {
    if (-not $CudaBinDir) {
        $cudaCandidates = @(
            "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.2\bin",
            "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.1\bin",
            "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.0\bin"
        )
        $CudaBinDir = $cudaCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    }
    if (-not $CudaBinDir) {
        throw "CUDA was requested, but no CUDA bin directory was found. Pass -CudaBinDir explicitly."
    }
    $cudaRuntimeNames = @("cublas64_13.dll", "cublasLt64_13.dll", "cudart64_13.dll")
    foreach ($cudaRuntimeName in $cudaRuntimeNames) {
        $cudaRuntimePath = Join-Path $CudaBinDir $cudaRuntimeName
        if (-not (Test-Path $cudaRuntimePath)) {
            throw "CUDA package is incomplete; missing $cudaRuntimePath"
        }
        $runtimeDlls += Get-Item $cudaRuntimePath
    }
}

Remove-Item $stagePath -Recurse -Force -ErrorAction SilentlyContinue
New-Item $stagePath -ItemType Directory -Force | Out-Null
New-Item (Join-Path $stagePath "models") -ItemType Directory -Force | Out-Null

Copy-Item (Join-Path $buildPath "FIR.exe") $stagePath
$runtimeDlls | Copy-Item -Destination $stagePath
Copy-Item $modelFile (Join-Path $stagePath "models\ggml-small.bin")
Copy-Item (Join-Path $repoRoot "web") $stagePath -Recurse
New-Item (Join-Path $stagePath "tools") -ItemType Directory -Force | Out-Null
Copy-Item (Join-Path $repoRoot "tools\nginx") (Join-Path $stagePath "tools") -Recurse
Remove-Item (Join-Path $stagePath "tools\nginx\logs") -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $stagePath "tools\nginx\temp") -Recurse -Force -ErrorAction SilentlyContinue
New-Item (Join-Path $stagePath "tools\nginx\logs") -ItemType Directory -Force | Out-Null
New-Item (Join-Path $stagePath "tools\nginx\temp") -ItemType Directory -Force | Out-Null
Set-Content (Join-Path $stagePath "tools\nginx\logs\.gitkeep") "" -Encoding ascii
Set-Content (Join-Path $stagePath "tools\nginx\temp\.gitkeep") "" -Encoding ascii
Copy-Item (Join-Path $repoRoot "LICENSE") $stagePath
Copy-Item (Join-Path $repoRoot "THIRD_PARTY_NOTICES.md") $stagePath
Copy-Item (Join-Path $repoRoot "README.md") $stagePath
@"
# FIR configuration
FIR_USE_GPU=$(if ($IncludeCuda -or $IncludeVulkan) { "1" } else { "0" })
QRZ_USER=
QRZ_PASS=
MY_CALLSIGN=
"@ | Set-Content (Join-Path $stagePath ".env") -Encoding ascii

$portablePath = Join-Path $distPath "FIR-$Version-$variant-portable.zip"
Remove-Item $portablePath -Force -ErrorAction SilentlyContinue
Compress-Archive -Path $stagePath -DestinationPath $portablePath -CompressionLevel Optimal

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::Open($portablePath, [System.IO.Compression.ZipArchiveMode]::Update)
try {
    $normalizedEnvPath = "FIR-$Version-$variant/.env"
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

& $InnoSetupPath "/DMyAppVersion=$Version" "/DSourceDir=$stagePath" "/DOutputDir=$distPath" "/DMyAppVariant=$variant" $installerScript
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup failed with exit code $LASTEXITCODE."
}

Write-Host "Created: $portablePath"
Write-Host "Created installer in: $distPath"