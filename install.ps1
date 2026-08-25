[CmdletBinding()]
param(
    [string]$KritaRoot = 'C:\Program Files\Krita (x64)'
)

$ErrorActionPreference = 'Stop'
$imagePluginDir = Join-Path $KritaRoot 'bin\imageformats'
$kritaPluginDir = Join-Path $KritaRoot 'lib\kritaplugins'
$sourcePlugin = Join-Path $PSScriptRoot 'bin\imageformats\kimg_vtf.dll'
$sourceMime = Join-Path $PSScriptRoot 'share\mime\packages\vtf.xml'
$userMimeDir = Join-Path $env:LOCALAPPDATA 'mime\packages'
$backupDir = Join-Path $KritaRoot ('vtf-plugin-backup-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))

if (-not (Test-Path -LiteralPath $sourcePlugin -PathType Leaf)) {
    throw "Missing build artifact: $sourcePlugin"
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

function Add-VtfMimeToFilter([string]$Path) {
    $oldText = 'image/x-xpixmap,image/x-xbitmap,'
    $newText = 'image/vnd.valve.source.texture,,'
    $old = [Text.Encoding]::ASCII.GetBytes($oldText)
    $new = [Text.Encoding]::ASCII.GetBytes($newText)
    if ($old.Length -ne $new.Length) {
        throw 'Internal metadata replacement lengths differ.'
    }
    $bytes = [IO.File]::ReadAllBytes($Path)
    function Find-ByteSequence([byte[]]$Haystack, [byte[]]$Needle) {
        $matches = New-Object Collections.Generic.List[int]
        for ($i = 0; $i -le $Haystack.Length - $Needle.Length; $i++) {
            $equal = $true
            for ($j = 0; $j -lt $Needle.Length; $j++) {
                if ($Haystack[$i + $j] -ne $Needle[$j]) { $equal = $false; break }
            }
            if ($equal) { $matches.Add($i) }
        }
        return $matches
    }
    $oldMatches = @(Find-ByteSequence $bytes $old)
    $newMatches = @(Find-ByteSequence $bytes $new)
    if ($oldMatches.Count -eq 1 -and $newMatches.Count -eq 0) {
        [Array]::Copy($new, 0, $bytes, $oldMatches[0], $new.Length)
        [IO.File]::WriteAllBytes($Path, $bytes)
        return $true
    }
    if ($oldMatches.Count -eq 0 -and $newMatches.Count -eq 1) {
        return $false
    }
    throw "Expected one original or already-patched QImageIO MIME span in $Path; found original=$($oldMatches.Count), patched=$($newMatches.Count)."
}

New-Item -ItemType Directory -Force $imagePluginDir, $userMimeDir, $backupDir | Out-Null
Copy-Item -Force $sourcePlugin $imagePluginDir
Copy-Item -Force $sourceMime (Join-Path $userMimeDir 'vtf.xml')

foreach ($name in @('kritaqimageioimport.dll', 'kritaqimageioexport.dll')) {
    $path = Join-Path $kritaPluginDir $name
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required Krita QImageIO bridge was not found: $path"
    }
    Copy-Item -Force $path $backupDir
    Add-VtfMimeToFilter $path
}

New-Item -Path 'HKCU:\Software\Classes\.vtf' -Force | Out-Null
Set-ItemProperty -Path 'HKCU:\Software\Classes\.vtf' -Name 'Content Type' -Value 'image/vnd.valve.source.texture'
Set-ItemProperty -Path 'HKCU:\Software\Classes\.vtf' -Name 'PerceivedType' -Value 'image'

Write-Host "Installed VTF support into $KritaRoot" -ForegroundColor Green
Write-Host "Backed up patched Krita filters to $backupDir" -ForegroundColor Yellow
Write-Host 'Start Krita and use File > Open or Save As with a .vtf file.'
