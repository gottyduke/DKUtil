#Requires -Version 5

# args
param (
    [string]$CF = $env:clang_format_instance
)

$CF

if (!(Test-Path $CF -PathType Leaf)) {
    "Failed to locate clang-format.exe"
    Exit
}

$headers = Get-ChildItem "$PSScriptRoot\include" -Recurse -File -ErrorAction SilentlyContinue | ? { $_.DirectoryName -inotmatch "external" }
$src = Get-ChildItem "$PSScriptRoot\test" -Recurse -File -ErrorAction SilentlyContinue | ? { $_.DirectoryName -inotmatch "configs" }

foreach ($file in $headers) {
    $file.BaseName
    & $CF -i -style=file $file
}

foreach ($file in $src) {
    $file.BaseName
    & $CF -i -style=file $file
}

"Formatted $($headers.Length) header files"
"Formatted $($src.Length) source files"