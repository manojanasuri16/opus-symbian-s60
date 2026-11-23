#ifndef TGOPUS_BENCH_H
#define TGOPUS_BENCH_H
#ifdef __cplusplus
extern "C" {
#endif
/* 0 = ok; negative = failure (-1 alloc/create, -2 encode, -3 decode, -4 insane output) */
int tgopus_selftest(void);
/* Encode+decode `frames` 20 ms mono VOIP frames; avg microseconds per frame. Returns 0 or <0. */
int tgopus_bench(int sample_rate, int complexity, int bitrate, int frames, int *enc_us, int *dec_us);
#ifdef __cplusplus
}
#endif
#endif
