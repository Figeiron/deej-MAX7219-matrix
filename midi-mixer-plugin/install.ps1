# Builds VolumeOsd.exe and installs the plugin into MIDI Mixer.
# Usage: powershell -ExecutionPolicy Bypass -File midi-mixer-plugin\install.ps1
$ErrorActionPreference = "Stop"

$pluginDir = $PSScriptRoot
$osdProject = Join-Path $pluginDir "..\volume-osd\VolumeOsd.csproj"
$binDir = Join-Path $pluginDir "bin"
$target = Join-Path $env:APPDATA "midi-mixer-app\plugins\com.figeiron.volume-osd"

# A running copy keeps the exe locked
Get-Process VolumeOsd -ErrorAction SilentlyContinue | Stop-Process -Force

# Framework-dependent: needs the .NET 10 Desktop Runtime, but the exe stays ~1 MB instead of ~160 MB
dotnet publish $osdProject -c Release -r win-x64 -p:SelfContained=false `
    -p:PublishSingleFile=true -p:DebugType=none -o $binDir
if ($LASTEXITCODE -ne 0) { throw "dotnet publish failed" }

New-Item -ItemType Directory -Force (Join-Path $target "bin") | Out-Null
Copy-Item (Join-Path $pluginDir "plugin.json"), (Join-Path $pluginDir "main.js") $target -Force
Copy-Item (Join-Path $binDir "VolumeOsd.exe") (Join-Path $target "bin") -Force

Write-Host "Installed to $target"
Write-Host "In MIDI Mixer: Plugins -> refresh, then enable 'Volume OSD'."
