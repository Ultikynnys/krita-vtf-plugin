[CmdletBinding()]
param(
    [string]$KritaRoot = 'C:\Program Files\Krita (x64)'
)

$ErrorActionPreference = 'Stop'
$pluginDir = Join-Path $KritaRoot 'lib\kritaplugins'
$mimeDir = Join-Path $KritaRoot 'share\mime\packages'
$sourcePluginDir = Join-Path $PSScriptRoot 'lib\kritaplugins'
$sourceMime = Join-Path $PSScriptRoot 'share\mime\packages\vtf.xml'

foreach ($file in @('kritavtfimport.dll', 'kritavtfexport.dll')) {
    $source = Join-Path $sourcePluginDir $file
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Missing build artifact: $source"
    }
}
if (-not (Test-Path -LiteralPath $sourceMime -PathType Leaf)) {
    throw "Missing MIME description: $sourceMime"
}
if (-not (Test-Path -LiteralPath (Join-Path $KritaRoot 'bin\krita.exe'))) {
    throw "Krita was not found at $KritaRoot"
}
if (Get-Process krita -ErrorAction SilentlyContinue) {
    throw 'Close Krita before installing the native VTF plugin.'
}

New-Item -ItemType Directory -Force $pluginDir, $mimeDir | Out-Null
Copy-Item -Force (Join-Path $sourcePluginDir 'kritavtfimport.dll') $pluginDir
Copy-Item -Force (Join-Path $sourcePluginDir 'kritavtfexport.dll') $pluginDir
Copy-Item -Force $sourceMime $mimeDir
Write-Host "Installed native VTF filters into $KritaRoot" -ForegroundColor Green
Write-Host 'Start Krita and use File > Open or Save As with a .vtf file.'
