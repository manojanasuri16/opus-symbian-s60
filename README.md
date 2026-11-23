# opus-symbian-s60

**Opus 1.3.1 (fixed-point) for Symbian S60 3rd Edition**, built as a static library
`tgopus.lib` with the open GnuPoc / CodeSourcery GCCE 3.4.3 toolchain, plus a self-test and
a benchmark that run on the phone and on a PC. It encodes and decodes real-time voice on a
**Nokia N79** (330 MHz ARM11) with no Opus source patches.

It was made for the voice calls of [RetroTele](https://github.com/manojanasuri16/RetroTele)
(a Telegram client for the N79); the Opus side is generic and useful for any S60 3rd Edition
(Symbian 9.x) native code that needs a modern speech codec.

## Status

| Claim | Evidence |
|---|---|
| Builds to `tgopus.lib` (528,080 bytes, armv5 urel) with GnuPoc + GCCE 3.4.3 + S60 3.2 SDK | rebuilt from this repo layout on 2026-11-23, no errors |
| Runs on a real **Nokia N79** (S60 3rd Edition FP2) | yes: self-test passed and benchmark measured on the phone (RetroTele 0.7.6, 2026-11-22); Opus then carried real Telegram voice calls |
| Other S60 3rd Edition devices (N95, E71, N82 ...) | **untested**, expected to work (same ARMv6/ARMv5 ABI), but speed will differ |
| S60 5th Edition / Symbian^3 / other compilers | untested (no reason it shouldn't compile; Symbian^3 can use the standard Opus build) |
| Host (x86) build with the same sources | works: `sh host_test.sh` (self-test passes) |

The Opus sources are **not** copied into this repo: `fetch_opus.sh` downloads the pristine
`opus-1.3.1.tar.gz` from xiph.org and checks its SHA-256. No patches.

## Speed on the N79

Measured on the phone with `tgopus_bench` (16 kHz mono, VoIP application, 16 kbit/s, 100
frames of a synthetic sine sweep plus noise, encode **and** decode of each 20 ms frame, wall
clock because the phone's `clock()` returns 0):

| Encoder complexity ("effort") | ms per 20 ms frame (encode + decode) | CPU share of real time |
|---|---|---|
| 0 | 5.6 | 28 % |
| 2 | 7.7 | 39 % |
| 5 | 11.5 | 58 % |

(The raw log lines of that run are not available any more; these numbers come from the result
screen the app showed and the notes taken at the time.) For comparison the same sources on an
x86 PC: encode 46 / 76 / 127 microseconds per frame and decode 8-9 microseconds (roughly
100x faster, `host_test.sh`). RetroTele's voice calls use 8 kHz mono at about 12 kbit/s
and 20-60 ms packets, which is cheaper than this 16 kHz benchmark. Opus decoders can output a lower sample rate than the stream was encoded at, which is how RetroTele decodes peers' packets at 8 kHz. Stereo, 48 kHz music and complexity 10 were not measured.

## How it was ported (and why it needed no patches)

`config/config.h` (hand written) plus `group/opus.mmp`:

| Setting | Why |
|---|---|
| `FIXED_POINT` | integer-only arithmetic is the sensible choice for a 330 MHz core and a GCCE 3.4.3 compiler (no float hardware is used) |
| `DISABLE_FLOAT_API` | no `opus_encode_float` etc.; removes float code |
| `OPUS_BUILD`, `HAVE_CONFIG_H` | standard |
| `VAR_ARRAYS` | GCC 3.4 supports C99 variable-length arrays in its default gnu89 mode; Opus then puts scratch on the stack and has **no global stack arena**, so there is no writable static data and the encoder/decoder are thread safe |
| no `-std=gnu99` | it makes `opus_types.h` pick `<stdint.h>`, which the Symbian `estlib` lacks; in default mode `__SYMBIAN32__` selects the right typedefs |
| no asm, no RTCD, no SIMD, no NEON/Ne10 | not available / needed |
| `OPTION GCCE -O2 -fomit-frame-pointer` | |

Practical tricks and gotchas:

- **Stack**: Opus's C99 arrays need roughly 16-32 KB of stack. An EXE built with
  `EPOCSTACKSIZE 0x10000` (64 KB) is fine on its main thread; inside a **PyS60 `.pyd`** you cannot
  rely on the interpreter thread's stack, so run Opus in its own `RThread` with a 64 KB stack and wait
  for it (see `docs/PORTING.md`).
- **Writable static data**: Symbian DLLs (such as a `.pyd`) cannot have it by default, and Opus has
  about **144 bytes** of it. Link it into an **EXE**, or add `EPOCALLOWDLLDATA` to the DLL's `.mmp`
  (that is what RetroTele's `tgcall.pyd` does).
- **Heap**: encoder and decoder states are allocated with `malloc` (via `estlib`), so give the exe
  a heap: `EPOCHEAPSIZE 0x20000 0x400000` was used for the call helper.
- `clock()` is 0 on the phone: time with `User::TickCount()` / `TTime::HomeTime()`.
- All 128 source files used are listed in `sources.txt` (CELT + SILK fixed + core). `group/opus.mmp`
  lists them for the Symbian build. Opus 1.3.1's `opus_projection_*` and `mapping_matrix.c` are
  compiled too (harmless).
- The N79's CPU frequency scales with load (probes sometimes read 192 MHz), so timings vary a little between runs.

## Build

You need the toolchain from
[symbian-s60v3-toolchain](https://github.com/manojanasuri16/symbian-s60v3-toolchain)
(GnuPoc + CSL GCCE 3.4.3 + the Nokia S60 3.2 SDK, which you must obtain yourself; see that repo).

```sh
SYMBIAN_SDK=/path/to/symbian-sdk sh build_opus.sh
# -> $SYMBIAN_SDK/s60_32/epoc32/release/armv5/urel/tgopus.lib   (build log in build.log)
```

Use it from another project:

```
// your_app.mmp (an EXE)
USERINCLUDE   <path>\opus-symbian-s60\src\include
STATICLIBRARY tgopus.lib
LIBRARY       estlib.lib
```

```c
#include "opus.h"
int err;
OpusEncoder *e = opus_encoder_create(8000, 1, OPUS_APPLICATION_VOIP, &err);
opus_encoder_ctl(e, OPUS_SET_BITRATE(12000));
opus_encoder_ctl(e, OPUS_SET_COMPLEXITY(2));
opus_encoder_ctl(e, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
int n = opus_encode(e, pcm160, 160, packet, sizeof packet);   /* 20 ms at 8 kHz */
```

Also exported by the library: `tgopus_selftest()` (encode+decode round trip with an energy and correlation
check; 0 = ok) and `tgopus_bench(rate, complexity, bitrate, frames, &enc_us, &dec_us)`
(`bench/tgopus_bench.h`). Remember `CloseSTDLIB()` when a thread that used libc exits.

## Test

```sh
sh host_test.sh        # builds the same sources + the bench for x86, runs selftest and benchmark
```

Last run (2026-11-23): `selftest: 0`; 16 kHz, 16 kbit/s: effort 0 enc 46 us / dec 8 us per frame, effort 2
76 / 8, effort 5 127 / 9. On the phone: see "Speed on the N79". The Symbian build was verified to compile and link
(`tgopus.lib` produced), but the library was **not** re-run on the phone from this new repository layout.

## Layout

```
config/config.h          hand-written Opus config
group/{bld.inf,opus.mmp} Symbian build (GCCE)
bench/                   tgopus_selftest / tgopus_bench (plain C)
sources.txt              source list for the host build
build_opus.sh            Symbian build (downloads Opus if needed)
fetch_opus.sh            downloads pristine opus-1.3.1 into src/ and verifies the SHA-256
host_test.sh             x86 build and run
docs/PORTING.md          notes on using it in a PyS60 .pyd and in an EXE
COPYING.opus             Opus licence (BSD 3-clause), kept
```

## Licence and credits

The files in this repo (config, mmp, bench, scripts) are MIT, see [LICENSE](LICENSE).
**Opus** is (c) Xiph.Org Foundation, Skype Limited, Octasic, Jean-Marc Valin, Timothy B.
Terriberry, CSIRO, Microsoft Corporation, Broadcom Corporation, Mozilla Corporation and
contributors, under the **BSD 3-clause licence** ([COPYING.opus](COPYING.opus)); that licence applies
to the downloaded sources and to any binary built from them, so keep that notice when you
redistribute `tgopus.lib` or programs that contain it. Opus is covered by royalty-free
patent grants from its contributors (see the notice in `COPYING.opus`). GnuPoc and the CSL
toolchain belong to their authors (see the toolchain repo).
