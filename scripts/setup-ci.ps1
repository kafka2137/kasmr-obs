# Pinned build tools for a clean Windows runner; no system-wide installation.
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$root = Split-Path -Parent $PSScriptRoot
$toolsDirectory = Join-Path $root 'reference/ci'
New-Item -ItemType Directory -Force $toolsDirectory | Out-Null

function Get-VerifiedDownload([string]$Uri, [string]$Destination, [string]$Sha256) {
    if (!(Test-Path -LiteralPath $Destination)) {
        Invoke-WebRequest -Uri $Uri -OutFile $Destination
    }
    $actual = (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash
    if ($actual -ine $Sha256) { throw "SHA256 mismatch for $Destination. Remove it and retry." }
}

$obsArchive = Join-Path $toolsDirectory 'OBS-Studio-32.0.4-Windows-x64.zip'
Get-VerifiedDownload `
    'https://github.com/obsproject/obs-studio/releases/download/32.0.4/OBS-Studio-32.0.4-Windows-x64.zip' `
    $obsArchive 'fd367a7f156319426f1a600720c1288ad0011da168dff41e0f94dbb18cc1dc0f'
$obsDirectory = Join-Path $toolsDirectory 'obs'
Expand-Archive -LiteralPath $obsArchive -DestinationPath $obsDirectory -Force
if (!(Test-Path "$obsDirectory/bin/64bit/obs64.exe")) { throw 'OBS runtime archive has an unexpected layout' }

$innoInstaller = Join-Path $toolsDirectory 'innosetup-6.7.3.exe'
Get-VerifiedDownload `
    'https://github.com/jrsoftware/issrc/releases/download/is-6_7_3/innosetup-6.7.3.exe' `
    $innoInstaller '9c73c3bae7ed48d44112a0f48e66742c00090bdb5bef71d9d3c056c66e97b732'
$innoDirectory = Join-Path $toolsDirectory 'inno'
$arguments = @('/PORTABLE=1', '/CURRENTUSER', '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', ('/DIR="' + $innoDirectory + '"'))
$process = Start-Process -FilePath $innoInstaller -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
if ($process.ExitCode) { throw "Inno Setup portable extraction failed: $($process.ExitCode)" }
if (!(Test-Path "$innoDirectory/ISCC.exe")) { throw 'Inno Setup compiler missing after extraction' }
Write-Output 'Prepared OBS Studio 32.0.4 and Inno Setup 6.7.3 for CI.'
