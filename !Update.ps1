# args
param (
    [Parameter(Mandatory)][ValidateSet('COPY', 'SOURCEGEN', 'DISTRIBUTE')][string]$Mode,
    [string]$Version,
    [string]$Path,
    [string]$Project,
    [string]$Anniversary # VS passes in string
)


$ErrorActionPreference = "Stop"

$Folder = $PSScriptRoot | Split-Path -Leaf
$AcceptedExt = @('.c', '.cpp', '.cxx', '.h', '.hpp', '.hxx')


function Resolve-Files {
    param (
        [Parameter(ValueFromPipeline)][string]$a_parent = $PSScriptRoot,
        [string[]]$a_directory = @('include', 'src', 'test')
    )
    
    process {
        Push-Location $PSScriptRoot
        $capacity = 16
        if ($Folder -eq 'CommonLibSSE') {
            $capacity = 2048
        } else {
            $capacity = 16
        }
        $_generated = [System.Collections.ArrayList]::new($capacity)

        try {
            foreach ($directory in $a_directory) {
                Get-ChildItem "$a_parent/$directory" -Recurse -File -ErrorAction SilentlyContinue | Where-Object {
                    ($_.Extension -in $AcceptedExt) -and 
                    ($_.Name -ne 'Version.h')
                } | Resolve-Path -Relative | ForEach-Object {
                    $_generated.Add("`n`t`"$($_.Substring(2) -replace '\\', '/')`"") | Out-Null
                }

                if (!$env:RebuildInvoke) {
                    Write-Host "`t<$a_parent/$directory>"
                    foreach ($file in $_generated) {
                        Write-Host "$file"
                    }
                }
            }
        } finally {
            Pop-Location
        }

        return $_generated
    }
}


Write-Host "`n`t<$Folder> [$Mode]"


# @@COPY
if ($Mode -eq 'COPY') {
    $GameBase = $null
    $MO2 = $null
    $Destination = $null

    # process newly added files
    $BuildFolder = Get-ChildItem (Get-Item $Path).Parent.Parent.FullName "$Project.sln" -Depth 2 -File -Exclude ('*CMakeFiles*', '*CLib*')
    $NewFiles = Get-ChildItem $BuildFolder.DirectoryName -File | Where-Object {$_.Extension -in $AcceptedExt}
    if ($NewFiles) { # trigger ZERO_CHECK
        $NewFiles | Move-Item -Destination "$PSScriptRoot/src" -Force -Confirm:$false -ErrorAction:SilentlyContinue | Out-Null
        [IO.File]::WriteAllText("$PSScriptRoot/CMakeLists.txt", [IO.File]::ReadAllText("$PSScriptRoot/CMakeLists.txt"))
    }

    # Build Target
    $AE = [bool][Int32]$Anniversary
    $vcpkg = [IO.File]::ReadAllText("$PSScriptRoot/vcpkg.json") | ConvertFrom-Json
    if ($AE) {
        $GameBase = $env:SkyrimAEPath
        $MO2 = $env:MO2SkyrimAEPath
        Write-Host "`t$Folder $Version | ANNIVERSARY EDITION"
    } else {
        $GameBase = $env:SkyrimSEPath
        $MO2 = $env:MO2SkyrimSEPath
        Write-Host "`t$Folder $Version | SPECIAL EDITION"
    }

    $BtnCopyDataText = $null
    if ($MO2) {
        $Destination = Join-Path "$MO2/mods" $vcpkg.'features'.'mo2-install'.'description'
        $BtnCopyDataText = 'Copy to MO2'
    } else {
        $Destination = Join-Path "$GameBase" "Data" 
        $BtnCopyDataText = 'Copy to Data'
    }

    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing

    [System.Windows.Forms.Application]::EnableVisualStyles()
    $MsgBox = New-Object System.Windows.Forms.Form -Property @{
        TopLevel = $true
        TopMost = $true
        ClientSize = '350, 250'
        Text = $Project
        StartPosition = 'CenterScreen'
        FormBorderStyle = 'FixedDialog'
        MaximizeBox = $false
        MinimizeBox = $false
        Font = New-Object System.Drawing.Font('Segoe UI', 10, [System.Drawing.FontStyle]::Regular)
    }
    
    $Message = New-Object System.Windows.Forms.Label -Property @{
        ClientSize = '190, 140'
        Location = New-Object System.Drawing.Point(20, 20)
        Text = "$Project has been built."
    }
    
    $BtnCopyData = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '90, 50'
        Location = New-Object System.Drawing.Point(20, 180)
        Text = $BtnCopyDataText
        Add_Click = {
            New-Item -Type Directory "$Destination/SKSE/Plugins" -Force | Out-Null

            # binary
            Copy-Item "$Path/$Project.dll" "$Destination/SKSE/Plugins/$Project.dll" -Force
            $Message.Text += "`nBinary file copied!"

            # configs
            Get-ChildItem $PSScriptRoot -Recurse | Where-Object {
                ($_.Extension -in '.toml', '.json', '.ini') -and 
                ($_.Name -ne 'vcpkg.json')
            } | ForEach-Object {
                Copy-Item $_.FullName "$Destination/SKSE/Plugins/$($_.Name)" -Force
                $Message.Text += "`n$($_.Name) copied!"
            }

            # shockwave
            if (Test-Path "$PSScriptRoot/Interface/*.swf" -PathType Leaf) {
                New-Item -Type Directory "$Destination/Interface" -Force | Out-Null
                Copy-Item "$PSScriptRoot/Interface" "$Destination" -Recurse -Force
                $Message.Text += "`nShockwave files copied!"
            }

            $BtnCopyData.Enabled = $false;
        }
    }

    $BtnLaunchSKSE = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '90, 50'
        Text = 'Launch SKSE'
        Location = New-Object System.Drawing.Point(130, 180)
        Add_Click = {
            Push-Location $GameBase
            Start-Process ./skse64_loader.exe
            Pop-Location
            $MsgBox.Close()
        }
    }
    
    $BtnOpenFolder = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '90, 50'
        Text = 'Open Folder'
        Location = New-Object System.Drawing.Point(240, 180)
        Add_Click = {
            Invoke-Item $Path
        }
    }

    $BtnExit = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '90, 50'
        Text = 'Exit'
        Location = New-Object System.Drawing.Point(240, 110)
        Add_Click = {
            $MsgBox.Close()
        }
    }

    # papyrus
    if (Test-Path "$PSScriptRoot/Scripts/Source/*.psc" -PathType Leaf) {
        $BtnBuildPapyrus = New-Object System.Windows.Forms.Button -Property @{
            ClientSize = '90, 50'
            Text = 'Build Papyrus'
            Location = New-Object System.Drawing.Point(240, 20)
            Add_Click = {
                New-Item -Type Directory "$Destination/Scripts" -Force | Out-Null
                & "$GameBase/Papyrus Compiler/PapyrusCompiler.exe" "$PSScriptRoot/Scripts/Source" -f="$GameBase/Papyrus Compiler/TESV_Papyrus_Flags.flg" -i="$GameBase/Data/Scripts/Source;./Scripts/Source" -o="$PSScriptRoot/Scripts" -a
    
                Copy-Item "$PSScriptRoot/Scripts" "$Destination" -Recurse -Force
                Remove-Item "$Destination/Scripts/Source" -Force -Confirm:$false -ErrorAction Ignore
                $Message.Text += "`nPapyrus scripts copied!"
            }
        }

        $MsgBox.Controls.Add($BtnBuildPapyrus)
    }
                
    $MsgBox.Controls.Add($Message)
    $MsgBox.Controls.Add($BtnCopyData)
    $MsgBox.Controls.Add($BtnLaunchSKSE)
    $MsgBox.Controls.Add($BtnOpenFolder)
    $MsgBox.Controls.Add($BtnExit)

    # Check CMake VERSION
    $OutputVersion
    $OriginalVersion = $vcpkg.'version-string'
    [IO.File]::ReadAllLines("$($BuildFolder.Directory)/include/Version.h") | ForEach-Object {
        if ($_.Trim().StartsWith('inline constexpr auto NAME = "')) {
            $OutputVersion = $_.Trim().Substring(30, 5)
            if ($OutputVersion -ne $vcpkg.'version-string') {
                $Message.Text += "`nVersionInfo changed! Updating CMakeLists..."  

                $CMakeLists = [IO.File]::ReadAllText("$PSScriptRoot/CMakeLists.txt") -replace "VERSION\s$($vcpkg.'version-string')", "VERSION $OutputVersion"
                [IO.File]::WriteAllText("$PSScriptRoot/CMakeLists.txt", $CMakeLists)

                $vcpkg.'version-string' = $OutputVersion
                $vcpkg = $vcpkg | ConvertTo-Json -Depth 9
                [IO.File]::WriteAllText("$PSScriptRoot/vcpkg.json", $vcpkg)
                
                $Message.Text += "`n$Project has been changed from $($OriginalVersion) to $($OutputVersion)`n`nThis update will be in effect after next successful build!"
            }
        }
    }

    $MsgBox.ShowDialog() | Out-Null
    Exit
}


# @@SOURCEGEN
if ($Mode -eq 'SOURCEGEN') {
    Write-Host "`tGenerating CMake sourcelist..."
    Remove-Item "$Path/sourcelist.cmake" -Force -Confirm:$false -ErrorAction Ignore

    $generated = 'set(SOURCES'
    $generated += $PSScriptRoot | Resolve-Files
    if ($Path) {
        $generated += $Path | Resolve-Files
    }
    $generated += "`n)"
    [IO.File]::WriteAllText("$Path/sourcelist.cmake", $generated)

    # update vcpkg.json accordinly
    $vcpkg = [IO.File]::ReadAllText("$PSScriptRoot/vcpkg.json") | ConvertFrom-Json
    $vcpkg.'name' = $vcpkg.'name'.ToLower()
    $vcpkg.'version-string' = $Version    
    if (!($vcpkg | Get-Member features)) {
        $features = @"
{
"mo2-install": {
    "description": ""
}
}
"@ | ConvertFrom-Json
        $features.'mo2-install'.'description' = $Folder
        $vcpkg | Add-Member -Name 'features' -Value $features -MemberType NoteProperty
    }

    # patch regression
    $vcpkg.PsObject.Properties.Remove('script-version')
    $vcpkg.PsObject.Properties.Remove('build-config')
    $vcpkg.PsObject.Properties.Remove('build-target')
    if ($vcpkg | Get-Member install-name) {
        $vcpkg.'features'.'mo2-install'.'description' = $vcpkg.'install-name'
    }
    $vcpkg.PsObject.Properties.Remove('install-name')

    # inversed version control
    if (Test-Path "$Path/version.rc" -PathType Leaf) {
        $VersionResource = [IO.File]::ReadAllText("$Path/version.rc") -replace "`"FileDescription`",\s`"$Folder`"",  "`"FileDescription`", `"$($vcpkg.'description')`""
        [IO.File]::WriteAllText("$Path/version.rc", $VersionResource)
    }

    $vcpkg = $vcpkg | ConvertTo-Json -Depth 9
    [IO.File]::WriteAllText("$PSScriptRoot/vcpkg.json", $vcpkg)
}


# @@DISTRIBUTE
if ($Mode -eq 'DISTRIBUTE') { # update script to every project
    Get-ChildItem "$PSScriptRoot/*/*" -Directory | Where-Object {
        $_.Name -notin @('vcpkg', 'Build', '.git', 'PluginTutorialCN') -and
        (Test-Path "$_/CMakeLists.txt" -PathType Leaf)
    } | ForEach-Object {
        Write-Host "`n`tUpdated <$_>"
        Robocopy.exe "$PSScriptRoot" "$_" '!Update.ps1' /MT /NJS /NFL /NDL /NJH | Out-Null
    }
}
