/* Opus feasibility benchmark for Symbian; plain C, no Python. */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "opus.h"
#include "tgopus_bench.h"

#define MAXPKT 1500
#define MAXRATE 48000

/* deterministic synthetic signal: sine sweep 200..3000 Hz + LCG noise */
static void gen(short *buf, int n, int rate, int start)
{
    unsigned seed = 12345u + (unsigned)start; /* no writable static data */
    int i;
    double ph, total = (double)rate;
    for (i = 0; i < n; i++) {
        double t = (double)(start + i) / total;           /* seconds */
        double v;
        ph = 2.0 * 3.14159265358979 * (200.0 * t + 1400.0 * t * t);
        seed = seed * 1664525u + 1013904223u;
        v = 8000.0 * sin(ph) + (double)((int)((seed >> 16) & 0x3ff) - 512) * 3.0;
        buf[i] = (short)v;
    }
}

static clock_t us_of(clock_t c) { return c; }

static OpusEncoder *mkenc(int rate, int cx, int br, int *err)
{
    OpusEncoder *e = opus_encoder_create(rate, 1, OPUS_APPLICATION_VOIP, err);
    if (!e || *err != OPUS_OK) return 0;
    opus_encoder_ctl(e, OPUS_SET_BITRATE(br));
    opus_encoder_ctl(e, OPUS_SET_COMPLEXITY(cx));
    opus_encoder_ctl(e, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    return e;
}

int tgopus_selftest(void)
{
    const int rate = 16000, fs = 320, nfr = 50;
    int err, i, rc = 0;
    OpusEncoder *e; OpusDecoder *d;
    short *in, *out; unsigned char pkt[MAXPKT];
    double ei = 0, eo = 0, best = 0;
    int lag;
    in = (short *)malloc(sizeof(short) * fs * nfr);
    out = (short *)malloc(sizeof(short) * fs * nfr);
    e = mkenc(rate, 5, 16000, &err);
    d = opus_decoder_create(rate, 1, &err);
    if (!in || !out || !e || !d) { rc = -1; goto done; }
    gen(in, fs * nfr, rate, 0);
    for (i = 0; i < nfr; i++) {
        int n = opus_encode(e, in + i * fs, fs, pkt, MAXPKT);
        if (n < 0) { rc = -2; goto done; }
        n = opus_decode(d, pkt, n, out + i * fs, fs, 0);
        if (n != fs) { rc = -3; goto done; }
    }
    /* energy check on second half + best normalised correlation over lag (codec delay) */
    for (i = fs * nfr / 2; i < fs * nfr; i++) { ei += (double)in[i] * in[i]; eo += (double)out[i] * out[i]; }
    if (eo < 0.1 * ei || eo > 4.0 * ei) { rc = -4; goto done; }
    for (lag = 0; lag < 400; lag += 2) {
        double c = 0, a = 0, b = 0;
        for (i = fs * nfr / 2; i < fs * nfr; i += 2) {
            double x = in[i - lag], y = out[i];
            c += x * y; a += x * x; b += y * y;
        }
        if (a > 0 && b > 0) { c = c / sqrt(a * b); if (c > best) best = c; }
    }
    if (best < 0.5) rc = -4;
done:
    if (e) opus_encoder_destroy(e);
    if (d) opus_decoder_destroy(d);
    free(in); free(out);
    return rc;
}

int tgopus_bench(int rate, int cx, int br, int frames, int *enc_us, int *dec_us)
{
    int fs = rate / 50, err, i, rc = 0, n, len = 0;
    OpusEncoder *e = 0; OpusDecoder *d = 0;
    short *in = 0, *out = 0; unsigned char *pk = 0; int *pl = 0;
    clock_t t0, t1;
    double te, td;
    if (frames < 1 || rate > MAXRATE) return -1;
    in = (short *)malloc(sizeof(short) * fs * frames);
    out = (short *)malloc(sizeof(short) * fs);
    pk = (unsigned char *)malloc(MAXPKT * frames);
    pl = (int *)malloc(sizeof(int) * frames);
    e = mkenc(rate, cx, br, &err);
    d = opus_decoder_create(rate, 1, &err);
    if (!in || !out || !pk || !pl || !e || !d) { rc = -1; goto done; }
    gen(in, fs * frames, rate, 0);
    t0 = clock();
    for (i = 0; i < frames; i++) {
        n = opus_encode(e, in + i * fs, fs, pk + i * MAXPKT, MAXPKT);
        if (n < 0) { rc = -2; goto done; }
        pl[i] = n; len += n;
    }
    t1 = clock();
    te = (double)(us_of(t1) - us_of(t0)) * 1e6 / CLOCKS_PER_SEC;
    t0 = clock();
    for (i = 0; i < frames; i++) {
        n = opus_decode(d, pk + i * MAXPKT, pl[i], out, fs, 0);
        if (n != fs) { rc = -3; goto done; }
    }
    t1 = clock();
    td = (double)(us_of(t1) - us_of(t0)) * 1e6 / CLOCKS_PER_SEC;
    if (enc_us) *enc_us = (int)(te / frames);
    if (dec_us) *dec_us = (int)(td / frames);
done:
    if (e) opus_encoder_destroy(e);
    if (d) opus_decoder_destroy(d);
    free(in); free(out); free(pk); free(pl);
    return rc;
}
