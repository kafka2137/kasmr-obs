param(
    [string]$IsccPath = '',
    [string]$SandboxOutput = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (!$IsccPath) {
    $candidates = @(
        (Join-Path $root 'reference/inno/ISCC.exe'),
        "${env:ProgramFiles(x86)}/Inno Setup 6/ISCC.exe",
        "$env:LOCALAPPDATA/Programs/Inno Setup 6/ISCC.exe"
    )
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) { $IsccPath = $command.Source }
    if (!$IsccPath) { $IsccPath = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1 }
}
if (!$IsccPath -or !(Test-Path -LiteralPath $IsccPath)) {
    throw 'Inno Setup compiler not found. Install Inno Setup 6.7+ and pass -IsccPath to ISCC.exe.'
}
$script = Join-Path $root 'build/installer-Windows.generated.iss'
if (!(Test-Path -LiteralPath $script)) { throw 'Run CMake configuration first to generate the installer script.' }
if (!(Test-Path "$root/dist/kasmr-obs/bin/64bit/kasmr-obs.dll")) { throw 'Stage the release first: cmake --install build --config Release --prefix dist' }
$arguments = @('/Qp', $script)
if ($SandboxOutput) {
    # Test artifact has no elevation, uninstall registration, or old-install migration.
    $arguments = @('/Qp', '/DSandboxBuild', "/O$SandboxOutput", '/Fkasmr-obs-test-installer', $script)
}
& $IsccPath @arguments
if ($LASTEXITCODE) { throw "Inno Setup compilation failed with exit code $LASTEXITCODE" }
if (!$SandboxOutput) { Write-Output "Installer created in $root/dist" }
