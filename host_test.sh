#!/bin/sh
# Host (x86) build of the same sources + bench; runs selftest and benchmark.
set -e
H=$(cd "$(dirname "$0")" && pwd)
O=${TMPDIR:-/tmp}/tgopus_host; mkdir -p "$O"
sh "$H/fetch_opus.sh"
SRC=$(sed "s#^#$H/src/#" "$H/sources.txt")
cat > "$O/main.c" <<'EOM'
#include <stdio.h>
#include "tgopus_bench.h"
int main(void){
  int cx[3]={0,2,5},i,e,d,r;
  printf("selftest: %d\n",tgopus_selftest());
  for(i=0;i<3;i++){ r=tgopus_bench(16000,cx[i],16000,500,&e,&d);
    printf("16000Hz cx=%d br=16000 rc=%d enc=%d us/frame dec=%d us/frame\n",cx[i],r,e,d);}
  return 0;}
EOM
gcc -O2 -w -std=gnu99 -DHAVE_CONFIG_H -DOPUS_BUILD -DFIXED_POINT -DDISABLE_FLOAT_API -DVAR_ARRAYS \
 -I"$H/config" -I"$H/src/include" -I"$H/src/celt" -I"$H/src/silk" -I"$H/src/silk/fixed" -I"$H/src" -I"$H/bench" \
 $SRC "$H/bench/tgopus_bench.c" "$O/main.c" -lm -o "$O/host_test"
"$O/host_test"
