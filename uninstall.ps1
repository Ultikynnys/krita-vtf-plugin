[CmdletBinding()]
param(
    [string]$KritaRoot = 'C:\Program Files\Krita (x64)'
)

$ErrorActionPreference = 'Stop'
$imagePluginDir = Join-Path $KritaRoot 'bin\imageformats'
$kritaPluginDir = Join-Path $KritaRoot 'lib\kritaplugins'
$userMimeDir = Join-Path $env:LOCALAPPDATA 'mime\packages'
$installedPlugin = Join-Path $imagePluginDir 'kimg_vtf.dll'
$installedExporter = Join-Path $kritaPluginDir 'kritavtfexport.dll'
$installedMime = Join-Path $userMimeDir 'vtf.xml'
$kritaExecutable = Join-Path $KritaRoot 'bin\krita.exe'

if (-not (Test-Path -LiteralPath $kritaExecutable -PathType Leaf)) {
    throw "Krita was not found at $KritaRoot"
}
if (Get-Process krita -ErrorAction SilentlyContinue) {
    throw 'Close Krita before uninstalling the native VTF plugin.'
}

function Set-VtfMimeOnFilter([string]$Path, [bool]$Enabled) {
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
    if ($Enabled -and $oldMatches.Count -eq 1 -and $newMatches.Count -eq 0) {
        [Array]::Copy($new, 0, $bytes, $oldMatches[0], $new.Length)
        [IO.File]::WriteAllBytes($Path, $bytes)
        return
    }
    if (-not $Enabled -and $oldMatches.Count -eq 0 -and $newMatches.Count -eq 1) {
        [Array]::Copy($old, 0, $bytes, $newMatches[0], $old.Length)
        [IO.File]::WriteAllBytes($Path, $bytes)
        return
    }
    if (($Enabled -and $newMatches.Count -eq 1) -or (-not $Enabled -and $oldMatches.Count -eq 1)) {
        return
    }
    throw "Expected one original or patched QImageIO MIME span in $Path; found original=$($oldMatches.Count), patched=$($newMatches.Count)."
}

foreach ($path in @($installedPlugin, $installedExporter, $installedMime)) {
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        Remove-Item -LiteralPath $path -Force
    }
}

foreach ($bridgeName in @('kritaqimageioimport.dll', 'kritaqimageioexport.dll')) {
    $path = Join-Path $kritaPluginDir $bridgeName
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        Write-Warning "Skipping missing Krita QImageIO bridge: $path"
        continue
    }
    Set-VtfMimeOnFilter $path $false
}

Remove-Item -Path 'HKCU:\Software\Classes\.vtf' -Recurse -Force -ErrorAction SilentlyContinue

Get-ChildItem -LiteralPath $KritaRoot -Directory -Filter 'vtf-plugin-backup-*' -ErrorAction SilentlyContinue |
    Remove-Item -Recurse -Force

Write-Host "Uninstalled VTF support from $KritaRoot" -ForegroundColor Green
Write-Host 'Restart Krita before reinstalling.' -ForegroundColor Yellow
