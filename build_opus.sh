#!/bin/sh
# Builds tgopus.lib (static) with the GnuPoc toolchain. See README.md.
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
S=${SYMBIAN_SDK:-$(cd "$HERE/.." && pwd)/symbian-sdk}   # a GnuPoc S60 3.2 setup, see README
export EPOCROOT="$S/s60_32/"
export PATH="$S/gnupoc-bin:$S/csl-gcc/bin:$PATH"
sh "$HERE/fetch_opus.sh"
cd "$HERE/group"
bldmake bldfiles >/dev/null 2>&1
abld build gcce urel > "$HERE/build.log" 2>&1 || true
grep -iE "error" "$HERE/build.log" || echo "no errors"
ls -la "$EPOCROOT"epoc32/release/*/urel/tgopus.lib
