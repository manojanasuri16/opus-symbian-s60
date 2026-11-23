# Using Opus in a PyS60 module or a Symbian EXE

## Where Opus runs

- **In an EXE (recommended)**: `EPOCSTACKSIZE 0x10000`, static link `tgopus.lib`, `LIBRARY estlib.lib`,
  call `opus_*` on the main thread. This is how RetroTele's call helper `tgcallaudio.exe` runs
  Opus: PCM16 8 kHz mono, 20 ms (320 byte) frames, Opus about 12 kbit/s (it needs the capabilities of
  its audio path; see [symbian-s60v3-media-notes](https://github.com/manojanasuri16/symbian-s60v3-media-notes)).
- **In a PyS60 `.pyd`**: possible, with two extra rules:
  1. add `EPOCALLOWDLLDATA` (Opus has ~144 bytes of writable data);
  2. do the Opus work on a **separate thread with a 64 KB stack**, because the Python thread's stack is
     too small for Opus's C99 arrays. Sketch (from RetroTele's `tgcall.pyd`, simplified):

```cpp
static TInt BenchThread(TAny* aArg)
{
    TBench* b = (TBench*)aArg;
    b->iRc = tgopus_bench(b->iRate, b->iComplexity, b->iBitrate, b->iFrames, &b->iEnc, &b->iDec);
    CloseSTDLIB();            // libc state of this thread
    return KErrNone;
}
// RThread t; t.Create(name, BenchThread, 64*1024 /*stack*/, NULL, &bench); t.Resume(); wait with Logon()
```

  Freeze the module's `.def` so `init<module>` stays export ordinal 1 (GCCE exports C++ vtables first).

## Capabilities

`tgopus.lib` itself needs none. The audio devices around it do: see the media notes repo.

## What was not tried

Stereo, 48 kHz, the float API, `OPUS_ARM_ASM` / NEON (not present on this CPU), and the S60 5th
Edition and Symbian^3. In RetroTele the decoder (8 kHz output) decoded the packets of the official Telegram apps during real calls.
