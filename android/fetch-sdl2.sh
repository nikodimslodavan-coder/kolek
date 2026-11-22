#!/usr/bin/env bash
# Скачивает исходники SDL2 рядом с Android-проектом (android/SDL2).
# Нужен только для сборки без интернета во время CMake-конфигурации:
# без этого скрипта CMake скачает SDL2 сам.
set -euo pipefail

VERSION="2.32.10"
SHA256="5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165"
HERE="$(cd "$(dirname "$0")" && pwd)"
DEST="$HERE/SDL2"
ARCHIVE="${TMPDIR:-/tmp}/SDL2-$VERSION.tar.gz"
URL="https://github.com/libsdl-org/SDL/releases/download/release-$VERSION/SDL2-$VERSION.tar.gz"

if [ -d "$DEST" ]; then
    echo "SDL2 уже скачан: $DEST"
    exit 0
fi

echo "Скачиваю SDL2 $VERSION..."
curl -L --fail --retry 3 -o "$ARCHIVE" "$URL"
echo "$SHA256  $ARCHIVE" | sha256sum -c -
tar -xzf "$ARCHIVE" -C "$HERE"
mv "$HERE/SDL2-$VERSION" "$DEST"
echo "Готово: $DEST"
