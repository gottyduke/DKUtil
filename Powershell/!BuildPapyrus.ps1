# ~path
$sseDir = "D:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition"
$sseSrc = $sseDir + "\Data\Scripts\Source"
$usrSrc = $PSScriptRoot
$papyrus = $sseDir + "\Papyrus Compiler\PapyrusCompiler.exe"
$flag = $sseDir + "\Papyrus Compiler\TESV_Papyrus_Flags.flg"


# ~compile all
& "$papyrus" "$usrSrc\Scripts\Source" -f="$flag" -i="$sseSrc;$usrSrc\Scripts\Source" -o="$usrSrc\Scripts" -a
