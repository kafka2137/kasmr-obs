param([string]$IsccPath = '', [string]$ObsExecutable = 'C:/Program Files/obs-studio/bin/64bit/obs64.exe')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$sandbox = Join-Path $root ('build/installer-tests/' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $sandbox | Out-Null
& "$root/scripts/package-windows.ps1" -IsccPath $IsccPath -SandboxOutput $sandbox
if ($LASTEXITCODE) { throw 'Test installer compilation failed' }
$installer = Join-Path $sandbox 'kasmr-obs-test-installer.exe'
function Invoke-TestInstaller([string]$Directory) {
    $arguments = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', ('/DIR="' + $Directory + '"'), ('/LOG="' + $sandbox + '/setup.log"'))
    (Start-Process -FilePath $installer -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru).ExitCode
}
$missingObs = Join-Path $sandbox 'missing-obs'
if ((Invoke-TestInstaller $missingObs) -eq 0) { throw 'Installer accepted a folder without OBS' }
if (Test-Path "$missingObs/obs-plugins/64bit/kasmr-obs.dll") { throw 'Invalid-folder check wrote plugin files' }
$obs = Join-Path $sandbox 'obs'
New-Item -ItemType Directory -Force "$obs/bin/64bit", "$obs/obs-plugins/64bit" | Out-Null
Copy-Item -LiteralPath $ObsExecutable -Destination "$obs/bin/64bit/obs64.exe"
Set-Content "$obs/obs-plugins/64bit/unrelated.txt" 'Preserve this other plugin'
$helper = Join-Path $root 'build/installer-process/Release/obs64.exe'
if (!(Test-Path $helper)) { throw 'Build the installer-process test target first.' }
$fakeObs = Start-Process -FilePath $helper -WindowStyle Hidden -PassThru
try {
    if ((Invoke-TestInstaller $obs) -eq 0) { throw 'Installer did not reject running OBS' }
    if (Test-Path "$obs/obs-plugins/64bit/kasmr-obs.dll") { throw 'Running-OBS guard wrote plugin files' }
} finally {
    if (!$fakeObs.HasExited) { Stop-Process -Id $fakeObs.Id }
}
if ((Invoke-TestInstaller $obs) -ne 0) { throw 'Installer failed for a valid OBS folder' }
$expected = (Get-FileHash "$root/dist/kasmr-obs/bin/64bit/kasmr-obs.dll").Hash
if ((Get-FileHash "$obs/obs-plugins/64bit/kasmr-obs.dll").Hash -ne $expected) { throw 'Installed DLL differs from release' }
if (!(Test-Path "$obs/data/obs-plugins/kasmr-obs/sounds/cream/press/SPACE.mp3")) { throw 'Sound pack was not installed' }
if ((Invoke-TestInstaller $obs) -ne 0) { throw 'Reinstall failed' }
$uninstaller = Get-ChildItem "$obs/data/obs-plugins/kasmr-obs/installer/unins*.exe" | Select-Object -First 1
if (!$uninstaller) { throw 'Uninstaller missing' }
$process = Start-Process -FilePath $uninstaller.FullName -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART') -WindowStyle Hidden -Wait -PassThru
if ($process.ExitCode) { throw 'Uninstall failed' }
if (Test-Path "$obs/obs-plugins/64bit/kasmr-obs.dll") { throw 'Uninstall left the plugin DLL behind' }
if (!(Test-Path "$obs/obs-plugins/64bit/unrelated.txt") -or !(Test-Path "$obs/bin/64bit/obs64.exe")) { throw 'Uninstall removed unrelated OBS files' }
Write-Output 'Installer tests passed: invalid folder and running OBS rejected; install, reinstall, and uninstall preserve OBS and unrelated files.'
