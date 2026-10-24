param([string]$PluginDirectory = "$env:ProgramData/obs-studio/plugins")
$ErrorActionPreference = 'Stop'
if (Get-Process obs64 -ErrorAction SilentlyContinue) { throw 'Close OBS Studio before installing.' }
$bundle = Join-Path $PSScriptRoot 'kasmr-obs'
if (!(Test-Path "$bundle/bin/64bit/kasmr-obs.dll")) { throw 'Extract the complete release ZIP before running install.ps1.' }
New-Item -ItemType Directory -Force -Path $PluginDirectory | Out-Null
Copy-Item -LiteralPath $bundle -Destination $PluginDirectory -Recurse -Force
Write-Output "Installed Kafka's Keyboard ASMR for OBS. Open OBS and add the Kafka's Keyboard ASMR for OBS source."
