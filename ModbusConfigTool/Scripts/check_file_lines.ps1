$root = Split-Path -Parent $PSScriptRoot
$patterns = @('*.h', '*.cpp', '*.ui', '*.qss')
$failed = $false

foreach ($pattern in $patterns)
{
    Get-ChildItem -LiteralPath $root -Recurse -File -Filter $pattern |
        Where-Object { $_.FullName -notmatch '\\build' } |
        ForEach-Object {
            $count = (Get-Content -LiteralPath $_.FullName).Count
            if ($count -gt 800)
            {
                Write-Error "$($_.FullName) contains $count lines (maximum: 800)."
                $failed = $true
            }
        }
}

if ($failed)
{
    exit 1
}

Write-Output 'All controlled files are within the 800-line limit.'
