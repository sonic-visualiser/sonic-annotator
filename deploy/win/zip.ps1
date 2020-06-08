
Set-StrictMode -Version 2.0
$ErrorActionPreference = "Stop"

Add-Type -assembly "system.io.compression.filesystem"

if ($args.length -ne 2) {
    echo "Usage: zip target.zip sourcedir"
    exit 2
}

$sourcename = $args[1]
$targetname = $args[0]

$source = (Resolve-Path $sourcename)

if ([System.IO.Path]::IsPathRooted($targetname)) {
   $target = $targetname
} else {
   $target = "$pwd\$targetname"
}

echo "Compressing from $source to $target..." | Out-Host
 
[io.compression.zipfile]::CreateFromDirectory($source, $target)

