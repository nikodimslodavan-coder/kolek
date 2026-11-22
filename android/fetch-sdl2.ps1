# Скачивает исходники SDL2 рядом с Android-проектом (android\SDL2).
# Нужен только для сборки без интернета во время CMake-конфигурации:
# без этого скрипта CMake скачает SDL2 сам.
$ErrorActionPreference = "Stop"

$Version = "2.32.10"
$Sha256  = "5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165"
$Here    = Split-Path -Parent $MyInvocation.MyCommand.Path
$Dest    = Join-Path $Here "SDL2"
$Archive = Join-Path $env:TEMP "SDL2-$Version.tar.gz"
$Url     = "https://github.com/libsdl-org/SDL/releases/download/release-$Version/SDL2-$Version.tar.gz"

if (Test-Path $Dest) {
    Write-Host "SDL2 уже скачан: $Dest"
    exit 0
}

Write-Host "Скачиваю SDL2 $Version..."
Invoke-WebRequest -Uri $Url -OutFile $Archive
$Hash = (Get-FileHash -Algorithm SHA256 $Archive).Hash.ToLower()
if ($Hash -ne $Sha256) {
    throw "Неверная контрольная сумма SDL2: $Hash"
}

tar -xzf $Archive -C $Here
Move-Item (Join-Path $Here "SDL2-$Version") $Dest
Write-Host "Готово: $Dest"
