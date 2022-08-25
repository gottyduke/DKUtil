# args
param (
    [Parameter(Mandatory)][ValidateSet('COPY', 'SOURCEGEN', 'DISTRIBUTE')][string]$Mode,
    [string]$Version,
    [string]$Path,
    [string]$Project
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
        $_generated = [System.Collections.ArrayList]::new()

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

    # process newly added files
    $BuildFolder = Get-ChildItem (Get-Item $Path).Parent.Parent.FullName "$Project.sln" -Depth 2 -File -Exclude ('*CMakeFiles*', '*f4se*')
    $NewFiles = Get-ChildItem $BuildFolder.DirectoryName -File | Where-Object {$_.Extension -in $AcceptedExt}
    if ($NewFiles) { # trigger ZERO_CHECK
        $NewFiles | Move-Item -($env:MO2Fallout4Path)/Data "$PSScriptRoot/src" -Force -Confirm:$false -ErrorAction:SilentlyContinue | Out-Null
        [IO.File]::WriteAllText("$PSScriptRoot/CMakeLists.txt", [IO.File]::ReadAllText("$PSScriptRoot/CMakeLists.txt"))
    }

    # Build Target
    Write-Host "`t$Folder $Version"
    $vcpkg = [IO.File]::ReadAllText("$PSScriptRoot/vcpkg.json") | ConvertFrom-Json

    $GameDir = Join-Path $env:Fallout4Path "Data"
    $MO2Dir = Join-Path "$($env:MO2Fallout4Path)/mods" $vcpkg.'features'.'mo2-install'.'description'

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
    
    $Message = New-Object System.Windows.Forms.TextBox -Property @{
        ClientSize = '225, 150'
        Location = New-Object System.Drawing.Point(20, 20)
        Multiline = $true
        ReadOnly = $true
        Text = "- [[ $Project ]] has been built."
        
    }

    function Copy-Mod {
        param (
            $Data
        )

        New-Item -Type Directory "$Data/F4SE/Plugins" -Force | Out-Null

        # binary
        Copy-Item "$Path/$Project.dll" "$Data/F4SE/Plugins/$Project.dll" -Force
        $Message.Text += "`r`n Binaries copied!"

        # configs
        Get-ChildItem $PSScriptRoot | Where-Object {
            ($_.Extension -in '.toml', '.json', '.ini') -and 
            ($_.Name -ne 'vcpkg.json')
        } | ForEach-Object {
            Copy-Item $_.FullName "$Data/F4SE/Plugins/$($_.Name)" -Force
            $Message.Text += "`r`n Configurations copied!"
        }

        # shockwave
        if (Test-Path "$PSScriptRoot/Interface/*.swf" -PathType Leaf) {
            New-Item -Type Directory "$Data/Interface" -Force | Out-Null
            Copy-Item "$PSScriptRoot/Interface" "$Data" -Recurse -Force
            $Message.Text += "`r`n Shockwave files copied!"
        }

        # papyrus
        if (Test-Path "$PSScriptRoot/Scripts/*.pex" -PathType Leaf) {
            New-Item -Type Directory "$Data/Scripts" -Force | Out-Null
            Copy-Item "$PSScriptRoot/Scripts" "$Data" -Recurse -Force
            $Message.Text += "`r`n Papyrus scripts copied!"
        }
    }

    $BtnCopyMO2 = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '70, 50'
        Location = New-Object System.Drawing.Point(260, 19)
        Text = '>>MO2'
        Add_Click = {
            Copy-Mod $MO2Dir
        }
    }

    $BtnCopyData = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '70, 50'
        Location = New-Object System.Drawing.Point(260, 74)
        Text = '>>Data'
        Add_Click = {
            Copy-Mod "$GameDir/Data"
        }
    }

    $BtnOpenFolder = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '70, 50'
        Text = 'Show'
        Location = New-Object System.Drawing.Point(260, 129)
        Add_Click = {
            Invoke-Item $Path
        }
    }

    $BtnLaunchF4SE = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '70, 50'
        Text = 'F4SE'
        Location = New-Object System.Drawing.Point(20, 185)
        Add_Click = {
            Push-Location $env:Fallout4Path
            Start-Process ./F4SE64_loader.exe
            Pop-Location
            $Message.Text += "`r`n F4SE Launched"
        }
    }

    $BtnBuildPapyrus = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '70, 50'
        Text = 'Papyrus'
        Location = New-Object System.Drawing.Point(100, 185)
        Add_Click = {
            $Message.Text += "`r`n Compiling papyrus scripts..."
            New-Item -Type Directory "$($env:Mo2Fallout4Path)/Data/Scripts" -Force | Out-Null
            & "$env:Fallout4Path/Papyrus Compiler/PapyrusCompiler.exe" "$PSScriptRoot/Scripts" -i="$GameDir/Data/Scripts/Source;$PSScriptRoot/Scripts;$PSScriptRoot/Scripts/Source" -o="$PSScriptRoot/Scripts" -a -op -enablecache -t="4"
        }
    }

    $BtnRemoveData = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '70, 50'
        Text = 'Remove'
        Location = New-Object System.Drawing.Point(180, 185)
        Add_Click = {
        }
    }

    $BtnExit = New-Object System.Windows.Forms.Button -Property @{
        ClientSize = '70, 50'
        Text = 'Exit'
        Location = New-Object System.Drawing.Point(260, 185)
        Add_Click = {
            $MsgBox.Close()
        }
    }

    if (!(Test-Path "$PSScriptRoot/Scripts/*.psc" -PathType Leaf)) {
        $BtnBuildPapyrus.Enabled = $false;
    }
                
    $MsgBox.Controls.Add($Message)
    $MsgBox.Controls.Add($BtnCopyData)
    $MsgBox.Controls.Add($BtnCopyMO2)
    $MsgBox.Controls.Add($BtnLaunchF4SE)
    $MsgBox.Controls.Add($BtnOpenFolder)
    $MsgBox.Controls.Add($BtnBuildPapyrus)
    $MsgBox.Controls.Add($BtnRemoveData)
    $MsgBox.Controls.Add($BtnExit)

    # Check CMake VERSION
    $OutputVersion
    $OriginalVersion = $vcpkg.'version-string'
    [IO.File]::ReadAllLines("$($BuildFolder.Directory)/include/Version.h") | ForEach-Object {
        if ($_.Trim().StartsWith('inline constexpr auto NAME = "')) {
            $OutputVersion = $_.Trim().Substring(30, 5)
            if ($OutputVersion -ne $vcpkg.'version-string') {
                $Message.Text += "`r`n VersionInfo changed! Updating CMakeLists..."  

                $CMakeLists = [IO.File]::ReadAllText("$PSScriptRoot/CMakeLists.txt") -replace "VERSION\s$($vcpkg.'version-string')", "VERSION $OutputVersion"
                [IO.File]::WriteAllText("$PSScriptRoot/CMakeLists.txt", $CMakeLists)

                $vcpkg.'version-string' = $OutputVersion
                $vcpkg = $vcpkg | ConvertTo-Json -Depth 9
                [IO.File]::WriteAllText("$PSScriptRoot/vcpkg.json", $vcpkg)
                
                $Message.Text += "`r`n $Project has been changed from $($OriginalVersion) to $($OutputVersion)`n`nThis update will be in effect after next successful build!"
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

    $Json = @'
{
    "name":  "",
    "version-string":  "1.0.0",
    "description":  "",
    "license":  "MIT",
    "dependencies":  [],
    "features": {
        "mo2-install": {
            "description": ""
        }
    }
}
'@ | ConvertFrom-Json
    
    # update vcpkg.json accordinly
    if (!(Test-Path "$PSScriptRoot/vcpkg.json")) {
        $Json.'name' = $Folder
        $vcpkg = $Json
    } else {
        $vcpkg = [IO.File]::ReadAllText("$PSScriptRoot/vcpkg.json") | ConvertFrom-Json
        $vcpkg.'name' = $vcpkg.'name'.ToLower()
    }
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

