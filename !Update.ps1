$ErrorActionPreference = "Stop"

function Resolve-Files {
    param (
        [Parameter(ValueFromPipeline)]
        [string]$a_parent = $PSScriptRoot,

        [Parameter()]
        [string[]]$a_directory = @('include', 'src', 'test'),

        [Parameter()]
        [string[]]$a_extension = '*.c,*.cpp,*.cxx,*.h,*.hpp,*.hxx'
    )
    
    process {
        Push-Location $PSScriptRoot # out of source invocation sucks
        $_generated = ""

        try {
            foreach ($directory in $a_directory) {
                Write-Host "`t<$a_parent/$directory>"
                Get-ChildItem "$a_parent/$directory" -Recurse -File -Include $a_extention -Exclude Version.h -ErrorAction SilentlyContinue | Resolve-Path -Relative | ForEach-Object {
                    Write-Host "`t`t<$_>"
                    $file = $_.SubString(2).Insert(0, "`n`t") -replace '\\', '/' # \\/\//\/\\/\\\/\\/?!?
                    $_generated = $_generated + $file
                }
            }
        } finally {
            Pop-Location
        }

        return $_generated
    }
}

# args
$a_mode = $args[0] ## COPY SOURCEGEN DISTRIBUTE
$a_version = $args[1]
$a_path = $args[2]
$a_project = $args[3]
$a_isAE = $args[4]

$GameBase
$Destination
$MO2

# project path
$Folder = $PSScriptRoot | Split-Path -Leaf

# operation
Write-Host "`n`t<$Folder> [$a_mode] BEGIN"
if ($a_mode -eq 'COPY') { # post build copy event
    
    $vcpkg = [IO.File]::ReadAllText("$PSScriptRoot/vcpkg.json") | ConvertFrom-Json
    if ($a_isAE -eq 'TRUE') {
        $GameBase = $env:SkyrimAEPath
        $MO2 = $env:MO2SkyrimAEPath
        Write-Host "`t$Folder $a_version | ANNIVERSARY EDITION"
    } elseif ($a_isAE -eq 'FALSE') {
        $GameBase = $env:SkyrimSEPath
        $MO2 = $env:MO2SkyrimSEPath
        Write-Host "`t$Folder $a_version | SPECIAL EDITION"
    }

    if ($MO2) {
        $Destination = Join-Path "$MO2/mods" $vcpkg.'install-name'
        New-Item -Type Directory "$Destination/SKSE/Plugins" -Force | Out-Null
        
        # binary
        Write-Host "`tCopying binary file..."
        Copy-Item "$a_path/$a_project.dll" "$Destination/SKSE/Plugins/$a_project.dll" -Force
        Write-Host "`tDone!"

        # configs
        Get-ChildItem "$PSScriptRoot" -Filter '*.ini,*json,*.toml' | ForEach-Object {
            Write-Host "`tCopying $($_.Name)"
            Copy-Item "$PSScriptRoot/$($_.Name)" "$Destination/SKSE/Plugins/$($_.Name)" -Force
        }

        # papyrus
        if (Test-Path "$PSScriptRoot/Scripts/Source/*.psc" -PathType Leaf) {
            Add-Type -AssemblyName Microsoft.VisualBasic | Out-Null
            $Result = [Microsoft.VisualBasic.Interaction]::MsgBox("Build papyrus scripts?", 52, 'Papyrus')
            if ($Result -eq 6) {
                Write-Host "`tBuilding papyrus scripts..."
                New-Item -Type Directory "$Destination/Scripts" -Force | Out-Null
                & "$GameBase/Papyrus Compiler/PapyrusCompiler.exe" "$PSScriptRoot/Scripts/Source" -f="$GameBase/Papyrus Compiler/TESV_Papyrus_Flags.flg" -i="$GameBase/Data/Scripts/Source;./Scripts/Source" -o="$PSScriptRoot/Scripts" -a

                Write-Host "`tCopying papyrus scripts..."
                Copy-Item "$PSScriptRoot/Scripts" "$Destination" -Recurse -Force
                Remove-Item "$Destination/Scripts/Source" -Force -Confirm:$false -ErrorAction Ignore
                Write-Host "`tDone!"
            }
        }

        # shockwave
        if (Test-Path "$PSScriptRoot/Interface/*.swf" -PathType Leaf) {
            Write-Host "`tCopying shockwave files..."
            New-Item -Type Directory "$Destination/Interface" -Force | Out-Null
            Copy-Item "$PSScriptRoot/Interface" "$Destination" -Recurse -Force
            Write-Host "`tDone!"
        }
    } else {
        Invoke-Item $a_path
    }
} elseif ($a_mode -eq 'SOURCEGEN') { # cmake sourcelist generation
    Write-Host "`tGenerating CMake sourcelist..."
    Remove-Item "$a_path/sourcelist.cmake" -Force -Confirm:$false -ErrorAction Ignore

    $generated = 'set(SOURCES'
    $generated += $PSScriptRoot | Resolve-Files
    if ($a_path) {
        $generated += $a_path | Resolve-Files
    }
    $generated += "`n)"
    [IO.File]::WriteAllText("$a_path/sourcelist.cmake", $generated)

    if ($a_version) {
        # update vcpkg.json accordinly
        $vcpkg = [IO.File]::ReadAllText("$PSScriptRoot/vcpkg.json") | ConvertFrom-Json
        $vcpkg.'version-string' = $a_version    
        
        if ($env:RebuildInvoke) {
            if ($vcpkg | Get-Member script-version) {
                $vcpkg.'script-version' = $env:DKScriptVersion
            } else {
                $vcpkg | Add-Member -Name 'script-version' -Value $env:DKScriptVersion -MemberType NoteProperty
            }
            if ($vcpkg | Get-Member build-config) {
                $vcpkg.'build-config' = $env:BuildConfig
            } else {
                $vcpkg | Add-Member -Name 'build-config' -Value $env:BuildConfig -MemberType NoteProperty
            }
            if ($vcpkg | Get-Member build-target) {
                $vcpkg.'build-target' = $env:BuildTarget
            } else {
                $vcpkg | Add-Member -Name 'build-target' -Value $env:BuildTarget -MemberType NoteProperty
            }
            if (-not ($vcpkg | Get-Member install-name)) {
                $vcpkg | Add-Member -Name 'install-name' -Value $Folder -MemberType NoteProperty
            }
        }
        $vcpkg = $vcpkg | ConvertTo-Json
        [IO.File]::WriteAllText("$PSScriptRoot/vcpkg.json", $vcpkg) # damn you encoding
    }
} elseif ($a_mode -eq 'DISTRIBUTE') { # update script to every project
    ((Get-ChildItem 'Plugins' -Directory -Recurse) + (Get-ChildItem 'Library' -Directory -Recurse)) | Resolve-Path -Relative | ForEach-Object {
        if (Test-Path "$_/CMakeLists.txt" -PathType Leaf) {
            Write-Host "`tUpdated <$_>"
            Robocopy.exe '.' "$_" '!Update.ps1' /MT /NJS /NFL /NDL /NJH
        }
    }
}

Write-Host "`t<$Folder> [$a_mode] DONE"
