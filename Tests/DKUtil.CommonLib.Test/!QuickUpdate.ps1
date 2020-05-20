$ErrorActionPreference = "Stop"

# ~args
$config = $args[0]
$extraCmd = $args[1]


# ~null
if (!$config) {
    Write-Host "        Invocation with Invalid Configuration Argument!" -ForegroundColor Red
    Exit
}


# ~current
$ProjDir = $PSScriptRoot
$Project = $PSScriptRoot | Split-Path -Leaf


# ~user path
$ModDir = "D:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\MO2\mods"
$GitDir = Join-Path -Path ([Environment]::GetFolderPath("Desktop")) -ChildPath "Workspace"
$Product = "D:\WorkSpace\skse64\x64\$config"

$ModPath = "$ModDir\$Project"
$GitPath = "$GitDir\$Project"


# ~touch
New-Item -Type dir "$ModPath\SKSE\Plugins" -Force | Out-Null
New-Item -Type dir "$GitPath\SKSE\Plugins" -Force | Out-Null
if (Test-Path "$ProjDir\Interface\*.swf" -PathType Leaf) {
    New-Item -Type dir "$ModPath\Interface" -Force | Out-Null
    New-Item -Type dir "$GitPath\Interface" -Force | Out-Null
}
if (Test-Path "$ProjDir\Scripts\Source\*.psc" -PathType Leaf) {
    New-Item -Type dir "$ModPath\Scripts" -Force | Out-Null
    New-Item -Type dir "$GirPath\Scripts" -Force | Out-Null
}


# ~binary
Write-Host "        Current Project <<< $Project >>>"
Copy-Item "$Product\$Project.dll" "$ModPath\SKSE\Plugins\$Project.dll" -Force
Copy-Item "$Product\$Project.dll" "$GitPath\SKSE\Plugins\$Project.dll" -Force
Write-Host "        Done!" -ForegroundColor Green


# ~debug
if ($config.Equals("Debug")) {
    Write-Host "        Copying debug symbols..." -ForegroundColor Yellow
    Copy-Item "$Product\$Project.dbg" "$ModPath\SKSE\Plugins\$Project.dbg" -Force
    Copy-Item "$Product\$Project.dbg" "$GitPath\SKSE\Plugins\$Project.dbg" -Force
    Write-Host "        Done!" -ForegroundColor Green
}


# ~json
if (Test-Path "$ProjDir\$Project.json" -PathType Leaf) {
    Write-Host "        Copying json settings..." -ForegroundColor Yellow
    Copy-Item "$ProjDir\$Project.json" "$ModPath\SKSE\Plugins\$Project.json" -Force
    Copy-Item "$ProjDir\$Project.json" "$GitPath\SKSE\Plugins\$Project.json" -Force
    Write-Host "        Done!" -ForegroundColor Green
}


# ~papyrus
if (Test-Path "$ProjDir\Scripts\Source\*.psc" -PathType Leaf) {
    Write-Host "        Building papyrus scripts..." -ForegroundColor Yellow
    $sseDir = "D:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition"
    $sseSrc = $sseDir + "\Data\Scripts\Source"
    $papyrus = $sseDir + "\Papyrus Compiler\PapyrusCompiler.exe"
    $flag = $sseDir + "\Papyrus Compiler\TESV_Papyrus_Flags.flg"

    & "$papyrus" "$ProjDir\Scripts\Source" -f="$flag" -i="$sseSrc;$ProjDir\Scripts\Source" -o="$ProjDir\Scripts" -a
    Write-Host "        Done!" -ForegroundColor Green

    Write-Host "        Copying papyrus scripts..." -ForegroundColor Yellow
    Copy-Item "$ProjDir\Scripts" "$ModPath" -Recurse -Force
    Copy-Item "$ProjDir\Scripts" "$GitPath" -Recurse -Force
    Write-Host "        Done!" -ForegroundColor Green
}


# ~shockwave
if (Test-Path "$ProjDir\Interface\*.swf" -PathType Leaf) {
    Write-Host "        Copying shockwave files..." -ForegroundColor Yellow
    Copy-Item "$ProjDir\Interface" "$ModPath" -Recurse -Force
    Copy-Item "$ProjDir\Interface" "$GitPath" -Recurse -Force
    Write-Host "        Done!" -ForegroundColor Green
}


# ~extra
if ($extraCmd) {
    Write-Host "        Executing extra commandlets..." -ForegroundColor Yellow
    & Invoke-Expression $extraCmd
    Write-Host "        Done!" -ForegroundColor Green
}


Write-Host "        Updated Project <<< $Project >>>" -ForegroundColor Green