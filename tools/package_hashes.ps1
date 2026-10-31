# Prints the SHA-256 of a release zip and of every file in the portable package folder (for the release notes).
#   powershell -ExecutionPolicy Bypass -File tools\package_hashes.ps1 -Package "<package folder>" [-Zip "<zip>"]
param(
    [Parameter(Mandatory = $true)][string]$Package,
    [string]$Zip = ""
)
$ErrorActionPreference = "Stop"
if ($Zip) {
    "{0}  {1}" -f (Get-FileHash -LiteralPath $Zip -Algorithm SHA256).Hash.ToLower(), (Split-Path $Zip -Leaf)
}
$root = (Resolve-Path -LiteralPath $Package).Path
Get-ChildItem -LiteralPath $root -Recurse -File | Sort-Object FullName | ForEach-Object {
    "{0}  {1}" -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower(), $_.FullName.Substring($root.Length + 1)
}
