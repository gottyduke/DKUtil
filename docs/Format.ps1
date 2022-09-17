$Header = '<a href="/docs/Config.md">Config</a> | <a href="/docs/Hook.md">Hook</a> | <a href="/docs/Logger.md">Logger</a> | <a href="/docs/Utility.md">Utility</a> | <a href="/docs/Extra.md">Extra</a></p>'

$Docs = Get-ChildItem $PSScriptRoot -File -Filter *.md -Recurse

foreach ($doc in $Docs) {
    $content = [IO.File]::ReadAllText($doc)
    
    $content = $content -replace '(?s)(?:(?<=\<p align\="center"\>\<a href\=)(.*?)(?=</p\>))', $Header
    
    [IO.File]::WriteAllText($doc, $content)

    "Formatted $($doc.Name)"
}    