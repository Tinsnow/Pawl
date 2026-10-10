/* Pawl version 1 -- reference implementation.  See pawl.h and SPEC.md. */
#include "pawl.h"
#include <string.h>

#define ROTL(x, r) (((x) << (r)) | ((x) >> (64 - (r))))

/* Round constant for round r (0-based) is (r+1) * PHI mod 2^64. */
#define PHI 0x9E3779B97F4A7C15ULL

/* Domain tags: the ASCII strings "Pawl1gen", "Pawl1key", "Pawl1mix",
 * "Pawl1end" read as little-endian 64-bit integers. */
#define TAG_GEN 0x6E6567316C776150ULL
#define TAG_KEY 0x79656B316C776150ULL
#define TAG_MIX 0x78696D316C776150ULL
#define TAG_END 0x646E65316C776150ULL

/*
 * One round on two rows of four words, a = (a0..a3) and b = (b0..b3):
 *
 *   a0   ^= round constant
 *   a[i] += b[i]                                    carries      (mod 2^64)
 *   b[i]  = rotl(b[i], R[i]) ^ a[i+1]               linear       (GF(2))
 *   a[i]  = rotl(a[i], Q[i]) + (b[i+1] & b[i+2])    AND + carries
 *   swap the rows
 *
 * with R = (18, 50, 58, 37) and Q = (34, 30, 54, 4), indices mod 4.
 * The swap is done by calling the macro with the rows exchanged.
 */
#define ROUND(a0, a1, a2, a3, b0, b1, b2, b3, rc)                        \
    do {                                                                 \
        a0 ^= (rc);                                                      \
        a0 += b0; a1 += b1; a2 += b2; a3 += b3;                          \
        b0 = ROTL(b0, 18) ^ a1; b1 = ROTL(b1, 50) ^ a2;                  \
        b2 = ROTL(b2, 58) ^ a3; b3 = ROTL(b3, 37) ^ a0;                  \
        a0 = ROTL(a0, 34) + (b1 & b2); a1 = ROTL(a1, 30) + (b2 & b3);    \
        a2 = ROTL(a2, 54) + (b3 & b0); a3 = ROTL(a3,  4) + (b0 & b1);    \
    } while (0)

static void permute(uint64_t s[8], int rounds)
{
    uint64_t a0 = s[0], a1 = s[1], a2 = s[2], a3 = s[3];
    uint64_t b0 = s[4], b1 = s[5], b2 = s[6], b3 = s[7];
    uint64_t rc = 0;
    int r;
    for (r = 0; r + 1 < rounds; r += 2) {
        rc += PHI; ROUND(a0, a1, a2, a3, b0, b1, b2, b3, rc);
        rc += PHI; ROUND(b0, b1, b2, b3, a0, a1, a2, a3, rc);
    }
    if (r < rounds) {                       /* odd round counts: tests only */
        uint64_t t;
        rc += PHI; ROUND(a0, a1, a2, a3, b0, b1, b2, b3, rc);
        t = a0; a0 = b0; b0 = t;  t = a1; a1 = b1; b1 = t;
        t = a2; a2 = b2; b2 = t;  t = a3; a3 = b3; b3 = t;
    }
    s[0] = a0; s[1] = a1; s[2] = a2; s[3] = a3;
    s[4] = b0; s[5] = b1; s[6] = b2; s[7] = b3;
}

void pawl_permute(uint64_t s[8], int rounds)
{
    permute(s, rounds);
}

/* Portable little-endian load/store. */
static uint64_t load64(const unsigned char *p)
{
    return (uint64_t)p[0]       | (uint64_t)p[1] << 8  | (uint64_t)p[2] << 16 |
           (uint64_t)p[3] << 24 | (uint64_t)p[4] << 32 | (uint64_t)p[5] << 40 |
           (uint64_t)p[6] << 48 | (uint64_t)p[7] << 56;
}
static void store64(unsigned char *p, uint64_t v)
{
    p[0] = (unsigned char)v;         p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16); p[3] = (unsigned char)(v >> 24);
    p[4] = (unsigned char)(v >> 32); p[5] = (unsigned char)(v >> 40);
    p[6] = (unsigned char)(v >> 48); p[7] = (unsigned char)(v >> 56);
}

/* Overwrite memory in a way the compiler may not optimise away: the call goes
 * through a volatile pointer, so it cannot be proved to be a dead store. */
static void *(*const volatile wipe_fn)(void *, int, size_t) = memset;
static void wipe(void *p, size_t n)
{
    wipe_fn(p, 0, n);
}

/*
 * One generator step.
 *
 *   (a, b)  = permute(key, (ctr_lo, ctr_hi, stream, TAG_GEN))
 *   out     = b + key        (word-wise, mod 2^64)
 *   key     = a ^ key
 *   ctr    += 1
 *
 * Nothing that leaves the permutation is shown unmasked, and the half that
 * becomes the next key is never output.
 */
static void step(pawl_t *g, uint64_t out[4])
{
    uint64_t s[8];
    s[0] = g->key[0]; s[1] = g->key[1]; s[2] = g->key[2]; s[3] = g->key[3];
    s[4] = g->ctr[0]; s[5] = g->ctr[1]; s[6] = g->stream;  s[7] = TAG_GEN;
    permute(s, PAWL_ROUNDS);
    out[0] = s[4] + g->key[0]; out[1] = s[5] + g->key[1];
    out[2] = s[6] + g->key[2]; out[3] = s[7] + g->key[3];
    g->key[0] ^= s[0]; g->key[1] ^= s[1]; g->key[2] ^= s[2]; g->key[3] ^= s[3];
    if (++g->ctr[0] == 0) ++g->ctr[1];
    wipe(s, sizeof s);
}

/*
 * Key derivation: a sponge over the same permutation.
 *
 *   a = key_in;  b = (len, 0, stream, tag)
 *   for each 32-byte block m of data (last one zero-padded): a ^= m; permute
 *   b3 ^= TAG_END; permute
 *   new key = a          (b is discarded, so the step cannot be inverted)
 */
static void absorb(pawl_t *g, uint64_t tag, const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char *)data;
    unsigned char block[32];
    uint64_t s[8];
    size_t left = len;
    int i;

    for (i = 0; i < 4; i++) s[i] = g->key[i];
    s[4] = (uint64_t)len; s[5] = 0; s[6] = g->stream; s[7] = tag;

    while (left > 0) {
        size_t n = left < 32 ? left : 32;
        memset(block, 0, sizeof block);
        memcpy(block, p, n);
        for (i = 0; i < 4; i++) s[i] ^= load64(block + 8 * i);
        permute(s, PAWL_ROUNDS);
        p += n; left -= n;
    }
    s[7] ^= TAG_END;
    permute(s, PAWL_ROUNDS);

    for (i = 0; i < 4; i++) g->key[i] = s[i];
    wipe(s, sizeof s);
    wipe(block, sizeof block);
}

void pawl_init(pawl_t *g, const void *seed, size_t len, uint64_t stream)
{
    memset(g, 0, sizeof *g);
    g->stream = stream;
    g->pos = 32;                             /* buffer empty */
    absorb(g, TAG_KEY, seed, len);
}

void pawl_init_u64(pawl_t *g, uint64_t seed)
{
    unsigned char b[8];
    store64(b, seed);
    pawl_init(g, b, sizeof b, 0);
}

void pawl_reseed(pawl_t *g, const void *data, size_t len)
{
    absorb(g, TAG_MIX, data, len);
    wipe(g->buf, sizeof g->buf);             /* drop output made under the old key */
    g->pos = 32;
}

void pawl_wipe(pawl_t *g)
{
    wipe(g, sizeof *g);
}

void pawl_fill(pawl_t *g, void *out, size_t len)
{
    unsigned char *o = (unsigned char *)out;
    uint64_t w[4];
    int i;

    /* drain what is left of the previous step, erasing as we go */
    while (len > 0 && g->pos < 32) {
        unsigned k = g->pos >> 3, sh = (g->pos & 7) * 8;
        *o++ = (unsigned char)(g->buf[k] >> sh);
        g->buf[k] &= ~((uint64_t)0xFF << sh);
        g->pos++; len--;
    }
    /* whole steps straight into the caller's buffer */
    while (len >= 32) {
        step(g, w);
        for (i = 0; i < 4; i++) store64(o + 8 * i, w[i]);
        o += 32; len -= 32;
    }
    /* tail: keep the unread part for next time */
    if (len > 0) {
        step(g, g->buf);
        g->pos = 0;
        while (len > 0) {
            unsigned k = g->pos >> 3, sh = (g->pos & 7) * 8;
            *o++ = (unsigned char)(g->buf[k] >> sh);
            g->buf[k] &= ~((uint64_t)0xFF << sh);
            g->pos++; len--;
        }
    }
    wipe(w, sizeof w);
}

uint64_t pawl_u64(pawl_t *g)
{
    unsigned char b[8];
    uint64_t v;
    if (g->pos >= 32) { step(g, g->buf); g->pos = 0; }
    if ((g->pos & 7) == 0) {                 /* fast path: a whole word is waiting */
        unsigned k = g->pos >> 3;
        v = g->buf[k]; g->buf[k] = 0; g->pos += 8;
        return v;
    }
    pawl_fill(g, b, 8);
    return load64(b);
}

uint32_t pawl_u32(pawl_t *g)
{
    unsigned char b[4];
    if (g->pos >= 32) { step(g, g->buf); g->pos = 0; }
    if ((g->pos & 3) == 0) {                 /* fast path: half a word is waiting */
        unsigned k = g->pos >> 3, sh = (g->pos & 4) * 8;
        uint32_t v = (uint32_t)(g->buf[k] >> sh);
        g->buf[k] &= ~((uint64_t)0xFFFFFFFF << sh);
        g->pos += 4;
        return v;
    }
    pawl_fill(g, b, 4);
    return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

double pawl_double(pawl_t *g)
{
    return (double)(pawl_u64(g) >> 11) * (1.0 / 9007199254740992.0);   /* 2^-53 */
}

/* Unbiased integer in [0, n): rejection sampling on the smallest bit mask
 * that covers n-1.  Portable (no 128-bit arithmetic); expected draws < 2. */
uint64_t pawl_below(pawl_t *g, uint64_t n)
{
    uint64_t mask, v;
    if (n <= 1) return 0;
    mask = n - 1;
    mask |= mask >> 1;  mask |= mask >> 2;  mask |= mask >> 4;
    mask |= mask >> 8;  mask |= mask >> 16; mask |= mask >> 32;
    do { v = pawl_u64(g) & mask; } while (v >= n);
    return v;
}

/* ---- operating-system entropy ------------------------------------------ */
#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
static int os_random(unsigned char *p, size_t n)
{
    return BCryptGenRandom(NULL, p, (ULONG)n, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 ? 0 : -1;
}
#else
#include <stdio.h>
static int os_random(unsigned char *p, size_t n)
{
    FILE *f = fopen("/dev/urandom", "rb");
    size_t got;
    if (!f) return -1;
    got = fread(p, 1, n, f);
    fclose(f);
    return got == n ? 0 : -1;
}
#endif

int pawl_init_os(pawl_t *g)
{
    unsigned char seed[32];
    if (os_random(seed, sizeof seed) != 0) { memset(g, 0, sizeof *g); return -1; }
    pawl_init(g, seed, sizeof seed, 0);
    wipe(seed, sizeof seed);
    return 0;
}
