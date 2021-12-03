# args
param (
    [ValidateSet('COPY', 'SOURCEGEN', 'DISTRIBUTE')][string]$Mode,
    [string]$Version,
    [string]$Path,
    [string]$Project,
    [string]$IsAE
)

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
                    $_generated += $file
                }
            }
        } finally {
            Pop-Location
        }

        return $_generated
    }
}

# project path
$Folder = $PSScriptRoot | Split-Path -Leaf
$GameBase
$Destination
$MO2

# operation
Write-Host "`n`t<$Folder> [$Mode] BEGIN"
if ($Mode -eq 'COPY') { # post build event
    $vcpkg = [IO.File]::ReadAllText("$PSScriptRoot/vcpkg.json") | ConvertFrom-Json
    # Build Target
    if ($IsAE -eq 'TRUE') {
        $GameBase = $env:SkyrimAEPath
        $MO2 = $env:MO2SkyrimAEPath
        Write-Host "`t$Folder $Version | ANNIVERSARY EDITION"
    } elseif ($IsAE -eq 'FALSE') {
        $GameBase = $env:SkyrimSEPath
        $MO2 = $env:MO2SkyrimSEPath
        Write-Host "`t$Folder $Version | SPECIAL EDITION"
    } else {
        Write-Host "`tUnknown build target specified!"
        Exit
    }

    # MO2 support
    if ($MO2) {
        $Destination = Join-Path "$MO2/mods" $vcpkg.'install-name'
    } else {
        Add-Type -AssemblyName Microsoft.VisualBasic | Out-Null
        $Result = [Microsoft.VisualBasic.Interaction]::MsgBox("$Project has been built`n`nCopy to game folder?", 52, $Project)
        if ($Result -eq 6) {
            $Destination = Join-Path "$GameBase" "Data" 
        } else {
            Invoke-Item $Path
            Exit
        }
    }
       
    New-Item -Type Directory "$Destination/SKSE/Plugins" -Force | Out-Null
    # binary
    Write-Host "`tCopying binary file..."
    Copy-Item "$Path/$Project.dll" "$Destination/SKSE/Plugins/$Project.dll" -Force
    Write-Host "`tDone!"

    # configs
    Get-ChildItem $PSScriptRoot -Recurse | Where-Object {($_.extension -in '.toml', '.json', '.toml') -and ($_.Name -ne 'vcpkg.json')} | ForEach-Object {
        Write-Host "`tCopying $($_.Name)"
        Copy-Item $_.FullName "$Destination/SKSE/Plugins/$($_.Name)" -Force
    }

    # papyrus
    if (Test-Path "$PSScriptRoot/Scripts/Source/*.psc" -PathType Leaf) {
        Add-Type -AssemblyName Microsoft.VisualBasic | Out-Null
        $Result = [Microsoft.VisualBasic.Interaction]::MsgBox('Build papyrus scripts?', 52, 'Papyrus')
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

    # Check CMake VERSION
    $OutputVersion
    $OriginalVersion = $vcpkg.'version-string'
    $BuildFolder = Get-ChildItem ((Get-Item $Path).Parent.Parent.FullName) "$Project.sln" -Recurse -File
    [IO.File]::ReadAllLines("$($BuildFolder.Directory)/include/Version.h") | ForEach-Object {
        if ($_.Trim().StartsWith('inline constexpr auto NAME = "')) {
            $OutputVersion = $_.Trim().Substring(30, 5)
            if ($OutputVersion -ne $vcpkg.'version-string') {
                Write-Host "`tVersionInfo changed! Updating CMakeLists..."  

                $CMakeLists = [IO.File]::ReadAllText("$PSScriptRoot/CMakeLists.txt") -replace "VERSION\s$($vcpkg.'version-string')", "VERSION $OutputVersion"
                [IO.File]::WriteAllText("$PSScriptRoot/CMakeLists.txt", $CMakeLists)

                $vcpkg.'version-string' = $OutputVersion
                $vcpkg = $vcpkg | ConvertTo-Json
                [IO.File]::WriteAllText("$PSScriptRoot/vcpkg.json", $vcpkg)
                Write-Host "`tDone  "
                
                Add-Type -AssemblyName Microsoft.VisualBasic | Out-Null
                [Microsoft.VisualBasic.Interaction]::MsgBox("$Project has been changed from $($OriginalVersion) to $($OutputVersion)`n`nThis update will be in effect after next succesful build", 48, 'Version Update')
                Exit
            }
        }
    }
} elseif ($Mode -eq 'SOURCEGEN') { # cmake sourcelist generation
    Write-Host "`tGenerating CMake sourcelist..."
    Remove-Item "$Path/sourcelist.cmake" -Force -Confirm:$false -ErrorAction Ignore

    $generated = 'set(SOURCES'
    $generated += $PSScriptRoot | Resolve-Files
    if ($Path) {
        $generated += $Path | Resolve-Files
    }
    $generated += "`n)"
    [IO.File]::WriteAllText("$Path/sourcelist.cmake", $generated)

    if ($Version) {
        # update vcpkg.json accordinly
        $vcpkg = [IO.File]::ReadAllText("$PSScriptRoot/vcpkg.json") | ConvertFrom-Json
        $vcpkg.'version-string' = $Version    
        
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
} elseif ($Mode -eq 'DISTRIBUTE') { # update script to every project
    ((Get-ChildItem 'Plugins' -Directory -Recurse) + (Get-ChildItem 'Library' -Directory -Recurse)) | Resolve-Path -Relative | ForEach-Object {
        if (Test-Path "$_/CMakeLists.txt" -PathType Leaf) {
            Write-Host "`tUpdated <$_>"
            Robocopy.exe "$PSScriptRoot" "$_" '!Update.ps1' /MT /NJS /NFL /NDL /NJH
        }
    }
}

Write-Host "`t<$Folder> [$Mode] DONE"

# SIG # Begin signature block
# MIIR2wYJKoZIhvcNAQcCoIIRzDCCEcgCAQExCzAJBgUrDgMCGgUAMGkGCisGAQQB
# gjcCAQSgWzBZMDQGCisGAQQBgjcCAR4wJgIDAQAABBAfzDtgWUsITrck0sYpfvNR
# AgEAAgEAAgEAAgEAAgEAMCEwCQYFKw4DAhoFAAQURNWSaHUMAZ1pFh8iZEk+eY5J
# M1mggg1BMIIDBjCCAe6gAwIBAgIQZAPCkAxHzpxOvoeEUruLiDANBgkqhkiG9w0B
# AQsFADAbMRkwFwYDVQQDDBBES1NjcmlwdFNlbGZDZXJ0MB4XDTIxMTIwMjEyMzYz
# MFoXDTIyMTIwMjEyNTYzMFowGzEZMBcGA1UEAwwQREtTY3JpcHRTZWxmQ2VydDCC
# ASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9d3xGpFZgLEPcI1mIG8OPB
# GjeIk3zIyaanh/Z7XRcL3kz21M5/k/hATY9JRjMzciJVLnFT46vW2DRCJrp0oKyA
# Uj2oE2jrvrUueS7Pu9WVVDN+nIWbW1lzlDutZ7uEMRaAQT8OgpsRTY/nA11Fvipb
# kwK4tgpAjMQdxrqstxB+nbV9AcsgRh4YdzpkjoDm2Di8CQw5pEaBw2wAJO1GxH+D
# UjU1xlbqRdgJVhjMMyg7p4LqwUQoZs7lAFINBwqC13m2qMr/m0lsgNny/0l8IRV/
# m5RyAihlc8KvZbk/4oWs5hZjaOc5PKKi+d4wPpNw2T799bJSjOFEAcvPtqoK80EC
# AwEAAaNGMEQwDgYDVR0PAQH/BAQDAgeAMBMGA1UdJQQMMAoGCCsGAQUFBwMDMB0G
# A1UdDgQWBBSxj44nD0I+OD6c+ON8obenSrsbczANBgkqhkiG9w0BAQsFAAOCAQEA
# J3YfLurEb8s51SBiDcuB2P00jHcZxYKwUNOqUfvjOUuvQu2UFKAbuM6y3ku6fMHC
# s5Sp/WKnxPsa4aN+TgEi4ZB1f8G8VOsxnJd45t53BcBppxDY+YnaaP+M9iH0c+Bv
# 5uKwl0+PwxsLyG1q2kTC7kjDO8zsBBwkHmksnZK7R7GgeStmftmylBaggFbbRAj9
# en0IJocxsDYpbUxevTvwlHFlw1FvUbotDeug6Rlz7v/UPslNEi4JaylIpBju72me
# AKkhNgwJyELUVr3iNQ1AG80QVaf6Yg6hzMcQTv1M/lOSl+wK+6SBgJ973eXT9FeJ
# +7lIvb7kxLaPhIOLBMi72DCCBP4wggPmoAMCAQICEA1CSuC+Ooj/YEAhzhQA8N0w
# DQYJKoZIhvcNAQELBQAwcjELMAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0
# IEluYzEZMBcGA1UECxMQd3d3LmRpZ2ljZXJ0LmNvbTExMC8GA1UEAxMoRGlnaUNl
# cnQgU0hBMiBBc3N1cmVkIElEIFRpbWVzdGFtcGluZyBDQTAeFw0yMTAxMDEwMDAw
# MDBaFw0zMTAxMDYwMDAwMDBaMEgxCzAJBgNVBAYTAlVTMRcwFQYDVQQKEw5EaWdp
# Q2VydCwgSW5jLjEgMB4GA1UEAxMXRGlnaUNlcnQgVGltZXN0YW1wIDIwMjEwggEi
# MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDC5mGEZ8WK9Q0IpEXKY2tR1zoR
# Qr0KdXVNlLQMULUmEP4dyG+RawyW5xpcSO9E5b+bYc0VkWJauP9nC5xj/TZqgfop
# +N0rcIXeAhjzeG28ffnHbQk9vmp2h+mKvfiEXR52yeTGdnY6U9HR01o2j8aj4S8b
# Ordh1nPsTm0zinxdRS1LsVDmQTo3VobckyON91Al6GTm3dOPL1e1hyDrDo4s1SPa
# 9E14RuMDgzEpSlwMMYpKjIjF9zBa+RSvFV9sQ0kJ/SYjU/aNY+gaq1uxHTDCm2mC
# tNv8VlS8H6GHq756WwogL0sJyZWnjbL61mOLTqVyHO6fegFz+BnW/g1JhL0BAgMB
# AAGjggG4MIIBtDAOBgNVHQ8BAf8EBAMCB4AwDAYDVR0TAQH/BAIwADAWBgNVHSUB
# Af8EDDAKBggrBgEFBQcDCDBBBgNVHSAEOjA4MDYGCWCGSAGG/WwHATApMCcGCCsG
# AQUFBwIBFhtodHRwOi8vd3d3LmRpZ2ljZXJ0LmNvbS9DUFMwHwYDVR0jBBgwFoAU
# 9LbhIB3+Ka7S5GGlsqIlssgXNW4wHQYDVR0OBBYEFDZEho6kurBmvrwoLR1ENt3j
# anq8MHEGA1UdHwRqMGgwMqAwoC6GLGh0dHA6Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9z
# aGEyLWFzc3VyZWQtdHMuY3JsMDKgMKAuhixodHRwOi8vY3JsNC5kaWdpY2VydC5j
# b20vc2hhMi1hc3N1cmVkLXRzLmNybDCBhQYIKwYBBQUHAQEEeTB3MCQGCCsGAQUF
# BzABhhhodHRwOi8vb2NzcC5kaWdpY2VydC5jb20wTwYIKwYBBQUHMAKGQ2h0dHA6
# Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydFNIQTJBc3N1cmVkSURUaW1l
# c3RhbXBpbmdDQS5jcnQwDQYJKoZIhvcNAQELBQADggEBAEgc3LXpmiO85xrnIA6O
# Z0b9QnJRdAojR6OrktIlxHBZvhSg5SeBpU0UFRkHefDRBMOG2Tu9/kQCZk3taaQP
# 9rhwz2Lo9VFKeHk2eie38+dSn5On7UOee+e03UEiifuHokYDTvz0/rdkd2NfI1Jp
# g4L6GlPtkMyNoRdzDfTzZTlwS/Oc1np72gy8PTLQG8v1Yfx1CAB2vIEO+MDhXM/E
# EXLnG2RJ2CKadRVC9S0yOIHa9GCiurRS+1zgYSQlT7LfySmoc0NR2r1j1h9bm/cu
# G08THfdKDXF+l7f0P4TrweOjSaH6zqe/Vs+6WXZhiV9+p7SOZ3j5NpjhyyjaW4em
# ii8wggUxMIIEGaADAgECAhAKoSXW1jIbfkHkBdo2l8IVMA0GCSqGSIb3DQEBCwUA
# MGUxCzAJBgNVBAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsT
# EHd3dy5kaWdpY2VydC5jb20xJDAiBgNVBAMTG0RpZ2lDZXJ0IEFzc3VyZWQgSUQg
# Um9vdCBDQTAeFw0xNjAxMDcxMjAwMDBaFw0zMTAxMDcxMjAwMDBaMHIxCzAJBgNV
# BAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdp
# Y2VydC5jb20xMTAvBgNVBAMTKERpZ2lDZXJ0IFNIQTIgQXNzdXJlZCBJRCBUaW1l
# c3RhbXBpbmcgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC90DLu
# S82Pf92puoKZxTlUKFe2I0rEDgdFM1EQfdD5fU1ofue2oPSNs4jkl79jIZCYvxO8
# V9PD4X4I1moUADj3Lh477sym9jJZ/l9lP+Cb6+NGRwYaVX4LJ37AovWg4N4iPw7/
# fpX786O6Ij4YrBHk8JkDbTuFfAnT7l3ImgtU46gJcWvgzyIQD3XPcXJOCq3fQDpc
# t1HhoXkUxk0kIzBdvOw8YGqsLwfM/fDqR9mIUF79Zm5WYScpiYRR5oLnRlD9lCos
# p+R1PrqYD4R/nzEU1q3V8mTLex4F0IQZchfxFwbvPc3WTe8GQv2iUypPhR3EHTyv
# z9qsEPXdrKzpVv+TAgMBAAGjggHOMIIByjAdBgNVHQ4EFgQU9LbhIB3+Ka7S5GGl
# sqIlssgXNW4wHwYDVR0jBBgwFoAUReuir/SSy4IxLVGLp6chnfNtyA8wEgYDVR0T
# AQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwEwYDVR0lBAwwCgYIKwYBBQUH
# AwgweQYIKwYBBQUHAQEEbTBrMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdp
# Y2VydC5jb20wQwYIKwYBBQUHMAKGN2h0dHA6Ly9jYWNlcnRzLmRpZ2ljZXJ0LmNv
# bS9EaWdpQ2VydEFzc3VyZWRJRFJvb3RDQS5jcnQwgYEGA1UdHwR6MHgwOqA4oDaG
# NGh0dHA6Ly9jcmw0LmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEFzc3VyZWRJRFJvb3RD
# QS5jcmwwOqA4oDaGNGh0dHA6Ly9jcmwzLmRpZ2ljZXJ0LmNvbS9EaWdpQ2VydEFz
# c3VyZWRJRFJvb3RDQS5jcmwwUAYDVR0gBEkwRzA4BgpghkgBhv1sAAIEMCowKAYI
# KwYBBQUHAgEWHGh0dHBzOi8vd3d3LmRpZ2ljZXJ0LmNvbS9DUFMwCwYJYIZIAYb9
# bAcBMA0GCSqGSIb3DQEBCwUAA4IBAQBxlRLpUYdWac3v3dp8qmN6s3jPBjdAhO9L
# hL/KzwMC/cWnww4gQiyvd/MrHwwhWiq3BTQdaq6Z+CeiZr8JqmDfdqQ6kw/4stHY
# fBli6F6CJR7Euhx7LCHi1lssFDVDBGiy23UC4HLHmNY8ZOUfSBAYX4k4YU1iRiSH
# Y4yRUiyvKYnleB/WCxSlgNcSR3CzddWThZN+tpJn+1Nhiaj1a5bA9FhpDXzIAbG5
# KHW3mWOFIoxhynmUfln8jA/jb7UBJrZspe6HUSHkWGCbugwtK22ixH67xCUrRwII
# fEmuE7bhfEJCKMYYVs9BNLZmXbZ0e/VWMyIvIjayS6JKldj1po5SMYIEBDCCBAAC
# AQEwLzAbMRkwFwYDVQQDDBBES1NjcmlwdFNlbGZDZXJ0AhBkA8KQDEfOnE6+h4RS
# u4uIMAkGBSsOAwIaBQCgeDAYBgorBgEEAYI3AgEMMQowCKACgAChAoAAMBkGCSqG
# SIb3DQEJAzEMBgorBgEEAYI3AgEEMBwGCisGAQQBgjcCAQsxDjAMBgorBgEEAYI3
# AgEVMCMGCSqGSIb3DQEJBDEWBBSSd4xP7G8jPHN8Wb81j3f8HZz8KDANBgkqhkiG
# 9w0BAQEFAASCAQBWIwwKqG0ZDikl3WUS0upCWZ7WglzVpB2vyLAamGtihwctJAbV
# KN8qslXFXMsye3DYLFvPUfN6bpUX4eKb+KhuxkCvc0AJm2abZCx51plUJQ0U92tR
# 48Dn857uP5FnBmG4dW9AuG1RNxLEEIPK+G8U+Fw7RU3wTiBheL9TuufXSyPFOuVp
# 6OiIsMmwvfNmbzhsU5j52fLpo+784SI79s8oORT3rnP/87xPHITzYd4BCZAE14IJ
# tG7mMO80Zsh4Up36nAAKK02nZAEKEIzhqtG1Ub/lXCmgC72zAEwblaIrg3O4QFrR
# i91IPzJumpEDBRfh7rTM0qXnuMsUigf4uEcmoYICMDCCAiwGCSqGSIb3DQEJBjGC
# Ah0wggIZAgEBMIGGMHIxCzAJBgNVBAYTAlVTMRUwEwYDVQQKEwxEaWdpQ2VydCBJ
# bmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5jb20xMTAvBgNVBAMTKERpZ2lDZXJ0
# IFNIQTIgQXNzdXJlZCBJRCBUaW1lc3RhbXBpbmcgQ0ECEA1CSuC+Ooj/YEAhzhQA
# 8N0wDQYJYIZIAWUDBAIBBQCgaTAYBgkqhkiG9w0BCQMxCwYJKoZIhvcNAQcBMBwG
# CSqGSIb3DQEJBTEPFw0yMTEyMDMxNjE1NTFaMC8GCSqGSIb3DQEJBDEiBCChKy0u
# an2QwfWy/257SPb2mWtdmFD4Hvf9vwt9hqFffTANBgkqhkiG9w0BAQEFAASCAQAQ
# YSCzdjMbyGpsDFn8aG2JelZgNLNEAkAaMm/MQSWzCuHkYcIQF0VachQFxqpr8VSm
# WP89UERlaCuFBisujojeiiLDQdV2kW8+EA45ghSmO75JxCWiUt8HWxN/O5rNt4M4
# ATVsWnFRElTwNM9nzBJeD/sDvv2xCDCDcfhP3JgsmcIsZJEamI2Z3mt1RllGyLWb
# Z2d5djvhaaf/vsa8SfwrQj6HIlD73o2icLinptG85UPlBwuXzldwm3hIEM8BiWad
# nr3AwPps3taN83i1xKvCaBXDAHNbQh8AqiOGI5xsx5fkvr9lfrkhVi7lyLvnZx/7
# XdbNTm0xy1PTwcUalauk
# SIG # End signature block
