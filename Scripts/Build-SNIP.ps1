$ErrorActionPreference = 'Stop'
$snipProjectRoot = Split-Path $PSScriptRoot -Parent
$snipEngine = 'C:\Program Files\Epic Games\UE_5.8'
& "$snipEngine\Engine\Build\BatchFiles\Build.bat" DirectorsCutJAMEditor Win64 Development "-Project=$snipProjectRoot\DirectorsCutJAM.uproject" -WaitMutex -NoHotReloadFromIDE -NoLiveCoding
if ($LASTEXITCODE -ne 0) { throw 'SNIP compilation failed.' }
# The saved map is already integrated. Rebuilding C++ does not regenerate it.
