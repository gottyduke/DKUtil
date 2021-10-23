$ErrorActionPreference = "Stop"

# args
$mode = $args[0]
$version = $args[1]
$path = $args[2]
$config = $args[3]

# project
$Project = $PSScriptRoot | Split-Path -Leaf

# user path
$ModPath = "$env:Skyrim64Path/MO2/mods/$Project"

if ($mode -eq "COPY") { # post build copy event
    # binary
    Write-Host "    Current <<< $Project $version >>>"
    New-Item -Type dir "$ModPath/SKSE/Plugins" -Force | Out-Null
    Copy-Item "$path/$config/$Project.dll" "$ModPath/SKSE/Plugins/$Project.dll" -Force
    Write-Host "    Done!"

    # settings
    if (Test-Path "$PSScriptRoot/$Project.json" -PathType Leaf) {
        Write-Host "    Copying json settings..."
        Copy-Item "$PSScriptRoot/$Project.json" "$ModPath/SKSE/Plugins/$Project.json" -Force
        Write-Host "    Done!"
    }

    # papyrus
    if (Test-Path "$PSScriptRoot/Scripts/Source/*.psc" -PathType Leaf) {
        Write-Host "    Building papyrus scripts..."
        New-Item -Type dir "$ModPath/Scripts" -Force | Out-Null
        & "$env:Skyrim64Path/Papyrus Compiler/PapyrusCompiler.exe" "$PSScriptRoot/Scripts/Source" -f="$env:Skyrim64Path/Papyrus Compiler/TESV_Papyrus_Flags.flg" -i="$env:Skyrim64Path/Data/Scripts/Source;./Scripts/Source" -o="$PSScriptRoot/Scripts" -a
        Write-Host "    Done!"

        Write-Host "    Copying papyrus scripts..."
        Copy-Item "$PSScriptRoot/Scripts" "$ModPath" -Recurse -Force
        Write-Host "    Done!"
    }

    # shockwave
    if (Test-Path "$PSScriptRoot/Interface/*.swf" -PathType Leaf) {
        Write-Host "    Copying shockwave files..."
        New-Item -Type dir "$ModPath/Interface" -Force | Out-Null
        Copy-Item "$PSScriptRoot/Interface" "$ModPath" -Recurse -Force
        Write-Host "    Done!"
    }
} elseif ($mode -eq "SOURCEGEN") { # cmake sourcelist generation
    Write-Host "    Generating CMake sourcelist..."
    Remove-Item "$PSScriptRoot/cmake/sourcelist.cmake" -ErrorAction Ignore
    Add-Content "$PSScriptRoot/cmake/sourcelist.cmake" -Value "set(SOURCES"

    try {
        Push-Location $PSScriptRoot # out of source invocation sucks

        # include
        $IncludeDir = Get-ChildItem "$PSScriptRoot/include" -Recurse -File -Include *.h,*.hpp,*.hxx -ErrorAction SilentlyContinue | Resolve-Path -Relative
        foreach ($file in $IncludeDir) {
            $file = $file.Substring(2).Insert(0, "`t") -replace '\\', '/' # \\/\//\/\\/\\\/\\/?!?
            Add-Content "$PSScriptRoot/cmake/sourcelist.cmake" -Value $file
        }
        
        # src
        $SrcDir = Get-ChildItem "$PSScriptRoot/src" -Recurse -File -Include *.c*,cpp,*.cxx -ErrorAction SilentlyContinue | Resolve-Path -Relative 
        foreach ($file in $SrcDir) {
            $file = $file.Substring(2).Insert(0, "`t") -replace '\\', '/'
            Add-Content "$PSScriptRoot/cmake/sourcelist.cmake" -Value $file
        }

        Write-Host "    Done!"
    } finally { 
        Pop-Location
    }

    Add-Content "$PSScriptRoot/cmake/sourcelist.cmake" -Value ")"

    # update vcpkg.json accordinly
    $vcpkg = Get-Content "$PSScriptRoot/vcpkg.json" | ConvertFrom-Json
    $vcpkg.'version-string' = $version
    $vcpkg = $vcpkg | ConvertTo-Json
    [IO.File]::WriteAllText("$PSScriptRoot/vcpkg.json", $vcpkg) # damn you encoding
}

Write-Host "    <<< $Project $version >>> Updated"
