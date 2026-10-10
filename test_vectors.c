/* Prints test vectors as "name: hex" lines.  compare_vectors.py prints the
 * same lines from the Python reference; the two outputs must be identical. */
#include "pawl.h"
#include <stdio.h>
#include <string.h>

static void hex(const char *name, const unsigned char *p, size_t n)
{
    size_t i;
    printf("%s: ", name);
    for (i = 0; i < n; i++) printf("%02x", p[i]);
    printf("\n");
}

int main(void)
{
    pawl_t g;
    unsigned char out[200], seed[100];
    uint64_t s[8];
    char name[64];
    int i, r;

    /* bare permutation on the all-zero state and on 0..7, several round counts */
    for (r = 1; r <= 24; r += (r < 4 ? 1 : (r == 4 ? 20 : 1))) {
        memset(s, 0, sizeof s);
        pawl_permute(s, r);
        sprintf(name, "permute zero r=%d", r);
        printf("%s:", name);
        for (i = 0; i < 8; i++) printf(" %016llx", (unsigned long long)s[i]);
        printf("\n");
    }
    for (i = 0; i < 8; i++) s[i] = (uint64_t)i;
    pawl_permute(s, 24);
    printf("permute 0..7 r=24:");
    for (i = 0; i < 8; i++) printf(" %016llx", (unsigned long long)s[i]);
    printf("\n");

    /* generator: empty seed */
    pawl_init(&g, "", 0, 0);
    pawl_fill(&g, out, 96);
    hex("seed empty, stream 0, 96 bytes", out, 96);

    /* seeds of every length 1..70 (bytes 0,1,2,...), 16 bytes each */
    for (i = 0; i < 100; i++) seed[i] = (unsigned char)i;
    for (i = 1; i <= 70; i++) {
        pawl_init(&g, seed, (size_t)i, 0);
        pawl_fill(&g, out, 16);
        sprintf(name, "seed len %d", i);
        hex(name, out, 16);
    }

    /* streams */
    pawl_init(&g, "pawl", 4, 1);
    pawl_fill(&g, out, 48);
    hex("seed 'pawl', stream 1", out, 48);
    pawl_init(&g, "pawl", 4, 0xFFFFFFFFFFFFFFFFULL);
    pawl_fill(&g, out, 48);
    hex("seed 'pawl', stream 2^64-1", out, 48);

    /* integer seed, mixed-size reads crossing step boundaries */
    pawl_init_u64(&g, 42);
    pawl_fill(&g, out, 5);
    hex("seed 42: 5 bytes", out, 5);
    printf("seed 42: then u64: %016llx\n", (unsigned long long)pawl_u64(&g));
    printf("seed 42: then u32: %08lx\n", (unsigned long)pawl_u32(&g));
    pawl_fill(&g, out, 77);
    hex("seed 42: then 77 bytes", out, 77);
    printf("seed 42: then u64: %016llx\n", (unsigned long long)pawl_u64(&g));
    for (i = 0; i < 5; i++) printf("seed 42: below(1000): %llu\n", (unsigned long long)pawl_below(&g, 1000));
    printf("seed 42: double: %.17g\n", pawl_double(&g));

    /* reseed */
    pawl_init_u64(&g, 7);
    pawl_fill(&g, out, 40);
    pawl_reseed(&g, "fresh entropy that is longer than one block, to cover two absorb blocks", 71);
    pawl_fill(&g, out, 48);
    hex("seed 7, 40 bytes, reseed, 48 bytes", out, 48);
    pawl_reseed(&g, "", 0);
    pawl_fill(&g, out, 32);
    hex("then empty reseed, 32 bytes", out, 32);

    /* long run: bytes 1,000,000 .. 1,000,031 of seed 'long' */
    pawl_init(&g, "long", 4, 0);
    for (i = 0; i < 5000; i++) pawl_fill(&g, out, 200);
    pawl_fill(&g, out, 32);
    hex("seed 'long', bytes 1000000..1000031", out, 32);
    return 0;
}
