$ErrorActionPreference = "Stop"

function Walk-Files {
    param (
        [Parameter(ValueFromPipeline)]
        [string]$a_parent = $PSScriptRoot,

        [Parameter()]
        [string[]]$a_directory = @('include', 'src'),

        [Parameter()]
        [string[]]$a_extension = '*.c,*.cpp,*.cxx,*.h,*.hpp,*.hxx'
    )
    
    process {
        Push-Location $PSScriptRoot # out of source invocation sucks
        $_generated = ""

        try {
            foreach ($directory in $a_directory) {
                Write-Host "`t<$a_parent/$directory>"
                $_tree = Get-ChildItem "$a_parent/$directory" -Recurse -File -Include $a_extention -Exclude Version.h -ErrorAction SilentlyContinue | Resolve-Path -Relative
                foreach ($leaf in $_tree) {
                    Write-Host "`t`t<$leaf>"
                    $leaf = $leaf.SubString(2).Insert(0, "`n`t") -replace '\\', '/' # \\/\//\/\\/\\\/\\/?!?
                    $_generated = $_generated + $leaf
                }
            }
        } finally {
            Pop-Location
        }

        return $_generated
    }
}

# args
$mode = $args[0] ## COPY SOURCEGEN DISTRIBUTE
$version = $args[1]
$path = $args[2]
$project = $args[3]

# project path
$Folder = $PSScriptRoot | Split-Path -Leaf
$ModPath = "$env:Skyrim64Path/MO2/mods/$project"

Write-Host "`n`t<$Folder> [$mode] BEGIN"
if ($mode -eq "COPY") { # post build copy event
    Write-Host "`tCurrent $Folder $version"

    # binary
    Write-Host "`tCopying binary file..."
    New-Item -Type dir "$ModPath/SKSE/Plugins" -Force | Out-Null
    Copy-Item "$path/$project.dll" "$ModPath/SKSE/Plugins/$project.dll" -Force
    Write-Host "`tDone!"

    # settings
    if (Test-Path "$PSScriptRoot/$project.ini" -PathType Leaf) {
        Write-Host "`tCopying ini configuration..."
        Copy-Item "$PSScriptRoot/$project.ini" "$ModPath/SKSE/Plugins/$project.ini" -Force
        Write-Host "`tDone!"
    }

    # papyrus
    if (Test-Path "$PSScriptRoot/Scripts/Source/*.psc" -PathType Leaf) {
        Write-Host "`tBuilding papyrus scripts..."
        New-Item -Type dir "$ModPath/Scripts" -Force | Out-Null
        & "$env:Skyrim64Path/Papyrus Compiler/PapyrusCompiler.exe" "$PSScriptRoot/Scripts/Source" -f="$env:Skyrim64Path/Papyrus Compiler/TESV_Papyrus_Flags.flg" -i="$env:Skyrim64Path/Data/Scripts/Source;./Scripts/Source" -o="$PSScriptRoot/Scripts" -a
        Write-Host "`tDone!"

        Write-Host "`tCopying papyrus scripts..."
        Copy-Item "$PSScriptRoot/Scripts" "$ModPath" -Recurse -Force
        Write-Host "`tDone!"
    }

    # shockwave
    if (Test-Path "$PSScriptRoot/Interface/*.swf" -PathType Leaf) {
        Write-Host "`tCopying shockwave files..."
        New-Item -Type dir "$ModPath/Interface" -Force | Out-Null
        Copy-Item "$PSScriptRoot/Interface" "$ModPath" -Recurse -Force
        Write-Host "`tDone!"
    }
} elseif ($mode -eq "SOURCEGEN") { # cmake sourcelist generation
    Write-Host "`tGenerating CMake sourcelist..."
    Remove-Item "$path/sourcelist.cmake" -Force -Confirm:$false -ErrorAction Ignore

    $generated = "set(SOURCES" 
    $generated += $PSScriptRoot | Walk-Files
    if ($path) {
        $generated += $path | Walk-Files
    }
    $generated += "`n)"
    [IO.File]::WriteAllText("$path/sourcelist.cmake", $generated)

    if ($version) {
        # update vcpkg.json accordinly
        $vcpkg = Get-Content "$PSScriptRoot/vcpkg.json" | ConvertFrom-Json
        $vcpkg.'version-string' = $version
        $vcpkg = $vcpkg | ConvertTo-Json
        [IO.File]::WriteAllText("$PSScriptRoot/vcpkg.json", $vcpkg) # damn you encoding
    }
} elseif ($mode -eq 'DISTRIBUTE') { # update script to every project
    $WorkSpaceDir = (Get-ChildItem "Plugins" -Directory -Recurse) + (Get-ChildItem "Library" -Directory -Recurse) | Resolve-Path -Relative
    foreach ($directory in $WorkSpaceDir) {
        if (Test-Path "$directory/CMakeLists.txt" -PathType Leaf) {
            Write-Host "`tUpdated <$directory>"
            Robocopy.exe "." "$directory" "!Update.ps1" /MT /NJS /NFL /NDL /NJH
        }
    }
}

Write-Host "`t<$Folder> [$mode] DONE"