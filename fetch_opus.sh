#!/bin/sh
# Downloads pristine Opus 1.3.1 into src/ (no patches are needed) and checks its SHA-256.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
[ -f "$HERE/src/include/opus.h" ] && exit 0
T="$HERE/opus-1.3.1.tar.gz"
curl -sSfL -o "$T" https://downloads.xiph.org/releases/opus/opus-1.3.1.tar.gz
echo "65b58e1e25b2a114157014736a3d9dfeaad8d41be1c8179866f144a2fb44ff9d  $T" | sha256sum -c -
mkdir -p "$HERE/src" && tar xzf "$T" -C "$HERE/src" --strip-components=1
rm -f "$T"
