/* Small self-test: seed avalanche, single-step avalanche, uniformity of the
 * derived outputs.  Prints a short report; exit code 0 if everything is in range. */
#include "../pawl.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int popcnt(const unsigned char *a, const unsigned char *b, int n)
{
    int i, c = 0;
    for (i = 0; i < n; i++) { unsigned x = a[i] ^ b[i]; while (x) { c += x & 1; x >>= 1; } }
    return c;
}

int main(void)
{
    pawl_t g, h;
    unsigned char seed[32], s2[32], o1[64], o2[64];
    int bit, i, bad = 0;
    double sum = 0, mn = 1e9, mx = 0;
    long trials = 0;

    /* 1. flip each bit of a 32-byte seed: how many of the first 512 output bits change? */
    pawl_init(&g, "selftest", 8, 0);
    for (i = 0; i < 64; i++) {
        pawl_fill(&g, seed, 32);
        pawl_init(&h, seed, 32, 0); pawl_fill(&h, o1, 64);
        for (bit = 0; bit < 256; bit++) {
            int d;
            memcpy(s2, seed, 32); s2[bit >> 3] ^= (unsigned char)(1u << (bit & 7));
            pawl_init(&h, s2, 32, 0); pawl_fill(&h, o2, 64);
            d = popcnt(o1, o2, 64); sum += d; trials++;
            if (d < mn) mn = d;
            if (d > mx) mx = d;
        }
    }
    printf("seed avalanche: flipping 1 seed bit changes %.2f of 512 output bits on average (ideal 256), range %.0f..%.0f over %ld trials\n",
           sum / trials, mn, mx, trials);
    if (fabs(sum / trials - 256) > 0.5 || mn < 190 || mx > 322) bad = 1;

    /* 2. stream ids: neighbouring stream ids give unrelated output */
    sum = 0; trials = 0;
    for (i = 0; i < 4096; i++) {
        pawl_init(&g, "selftest", 8, (uint64_t)i);     pawl_fill(&g, o1, 64);
        pawl_init(&g, "selftest", 8, (uint64_t)i + 1); pawl_fill(&g, o2, 64);
        sum += popcnt(o1, o2, 64); trials++;
    }
    printf("stream avalanche: stream n vs n+1 differ in %.2f of 512 bits on average (ideal 256)\n", sum / trials);
    if (fabs(sum / trials - 256) > 1.0) bad = 1;

    /* 3. pawl_below: chi-square against uniform for a few awkward ranges */
    {
        static const uint64_t ns[] = {3, 6, 10, 1000, 65537};
        unsigned k;
        for (k = 0; k < sizeof ns / sizeof ns[0]; k++) {
            static long cnt[65537];
            uint64_t n = ns[k]; long N = 200L * (long)n + 2000000, j; double chi = 0, e, z;
            memset(cnt, 0, sizeof cnt);
            for (j = 0; j < N; j++) cnt[pawl_below(&g, n)]++;
            e = (double)N / (double)n;
            for (j = 0; j < (long)n; j++) chi += (cnt[j] - e) * (cnt[j] - e) / e;
            z = (chi - (double)(n - 1)) / sqrt(2.0 * (double)(n - 1));
            printf("below(%llu): chi-square z = %+.2f over %ld draws (|z| < 4 expected)\n", (unsigned long long)n, z, N);
            if (fabs(z) > 4.5) bad = 1;
        }
    }

    /* 4. pawl_double: range, mean, variance, 1024-bin chi-square */
    {
        long N = 20000000, j; static long bins[1024]; double m = 0, v = 0, chi = 0, lo = 1, hi = 0, e, z;
        for (j = 0; j < N; j++) { double x = pawl_double(&g); m += x; v += x * x; if (x < lo) lo = x; if (x > hi) hi = x; bins[(int)(x * 1024)]++; }
        m /= N; v = v / N - m * m; e = N / 1024.0;
        for (j = 0; j < 1024; j++) chi += (bins[j] - e) * (bins[j] - e) / e;
        z = (chi - 1023) / sqrt(2046.0);
        printf("double: min %.3g max %.9f mean %.6f (0.5) variance %.6f (0.083333) chi-square z = %+.2f\n", lo, hi, m, v, z);
        if (lo < 0 || hi >= 1 || fabs(m - 0.5) > 0.0004 || fabs(v - 1.0 / 12) > 0.0003 || fabs(z) > 4.5) bad = 1;
    }
    printf(bad ? "SELFTEST FAILED\n" : "selftest ok\n");
    return bad;
}
