param(
    [switch]$Bootstrap,
    [string]$ObsRuntime = 'C:/Program Files/obs-studio/bin/64bit',
    [string]$ObsSource = '',
    [string]$SdkPath = '',
    [switch]$Installer,
    [string]$IsccPath = '',
    [string]$ReleaseTag = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
    if ($Bootstrap -and !(Test-Path reference/obs-studio/libobs/obs-module.h)) {
        git clone --depth 1 --branch 32.0.4 https://github.com/obsproject/obs-studio.git reference/obs-studio
        if ($LASTEXITCODE) { throw 'Failed to download OBS headers' }
    }
    $options = @('-S', '.', '-B', 'build', '-A', 'x64', "-DOBS_RUNTIME_DIR=$ObsRuntime")
    if ($ObsSource) { $options += "-DOBS_SOURCE_DIR=$ObsSource" }
    if ($SdkPath) { $options += "-DCMAKE_PREFIX_PATH=$SdkPath" }
    cmake @options
    if ($LASTEXITCODE) { throw 'CMake configuration failed' }
    $version = (Get-Content build/project-version.txt -Raw).Trim()
    if ($ReleaseTag -and $ReleaseTag -cne "v$version") {
        throw "Release tag '$ReleaseTag' must match the CMake project version: v$version"
    }
    cmake --build build --config Release
    if ($LASTEXITCODE) { throw 'Build failed' }
    ctest --test-dir build -C Release --output-on-failure
    if ($LASTEXITCODE) { throw 'Core tests failed' }
    $savedPath = $env:PATH
    try {
        $env:PATH = "$ObsRuntime;$savedPath"
        & ./build/Release/obs-smoke.exe "$root/build/Release/kasmr-obs.dll" "$root/data"
        if ($LASTEXITCODE) { throw 'OBS runtime smoke test failed' }
        & ./build/Release/obs-smoke.exe "$root/build/Release/kasmr-obs.dll" "$root/data" 44100
        if ($LASTEXITCODE) { throw 'OBS 44.1 kHz smoke test failed' }
    } finally { $env:PATH = $savedPath }
    cmake --install build --config Release --prefix dist
    if ($LASTEXITCODE) { throw 'Packaging failed' }
    if ($Installer) {
        & "$root/tests/installer-tests.ps1" -IsccPath $IsccPath -ObsExecutable (Join-Path $ObsRuntime 'obs64.exe')
        & "$PSScriptRoot/package-windows.ps1" -IsccPath $IsccPath
    }
    Copy-Item scripts/install.ps1 dist/install.ps1 -Force
    Copy-Item README.md dist/README.md -Force
    Compress-Archive -Path dist/kasmr-obs,dist/install.ps1,dist/README.md -DestinationPath "dist/kasmr-obs-$version-windows-x64.zip" -Force
    $sourceFiles = @('CMakeLists.txt', 'README.md', 'CHANGELOG.md', 'LICENSE', 'THIRD_PARTY_NOTICES.md', '.gitignore', '.github', 'src', 'cmake', 'scripts', 'tests', 'third_party', 'data')
    Compress-Archive -Path $sourceFiles -DestinationPath "dist/kasmr-obs-$version-source.zip" -Force
    Write-Output "Built and verified: $root/dist/kasmr-obs-$version-windows-x64.zip"
} finally { Pop-Location }
