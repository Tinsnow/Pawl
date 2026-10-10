/* Writes an endless stream of bytes to stdout, for feeding test batteries.
 *
 *   pawl_stream gen [seed-text] [stream-id]   the generator itself
 *   pawl_stream ctr                           bare permutation of a counter, no key, no
 *                                             feed-forward, all 512 output bits (harshest)
 *   pawl_stream seeds                         first 32 bytes from seeds 0,1,2,3,...
 *   pawl_stream streams [n]                   n streams of one seed, 8 bytes each in turn
 *   pawl_stream bytes  [seed-text]            generator read through odd-sized fills
 *
 * Build with -DPAWL_ROUNDS=n to test round-reduced versions.
 */
#include "../pawl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK 65536
static unsigned char buf[CHUNK];

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "gen";
    pawl_t g;
    if (!strcmp(mode, "gen")) {
        const char *seed = argc > 2 ? argv[2] : "";
        uint64_t stream = argc > 3 ? strtoull(argv[3], 0, 0) : 0;
        pawl_init(&g, seed, strlen(seed), stream);
        for (;;) { pawl_fill(&g, buf, CHUNK); if (fwrite(buf, 1, CHUNK, stdout) != CHUNK) return 0; }
    } else if (!strcmp(mode, "ctr")) {
        uint64_t c = 0, s[8]; size_t n = 0;
        for (;;) {
            memset(s, 0, sizeof s); s[4] = c++;
            pawl_permute(s, PAWL_ROUNDS);
            memcpy(buf + n, s, 64); n += 64;
            if (n == CHUNK) { if (fwrite(buf, 1, CHUNK, stdout) != CHUNK) return 0; n = 0; }
        }
    } else if (!strcmp(mode, "seeds")) {
        uint64_t seed = 0; size_t n = 0;
        for (;;) {
            pawl_init_u64(&g, seed++);
            pawl_fill(&g, buf + n, 32); n += 32;
            if (n == CHUNK) { if (fwrite(buf, 1, CHUNK, stdout) != CHUNK) return 0; n = 0; }
        }
    } else if (!strcmp(mode, "streams")) {
        int ns = argc > 2 ? atoi(argv[2]) : 64, i; size_t n = 0;
        pawl_t *gs = malloc(sizeof(pawl_t) * (size_t)ns);
        for (i = 0; i < ns; i++) pawl_init(&gs[i], "streams", 7, (uint64_t)i);
        for (;;) {
            for (i = 0; i < ns; i++) {
                uint64_t v = pawl_u64(&gs[i]);
                memcpy(buf + n, &v, 8); n += 8;
                if (n == CHUNK) { if (fwrite(buf, 1, CHUNK, stdout) != CHUNK) return 0; n = 0; }
            }
        }
    } else if (!strcmp(mode, "bytes")) {
        const char *seed = argc > 2 ? argv[2] : "";
        size_t n = 0, k = 1;
        pawl_init(&g, seed, strlen(seed), 0);
        for (;;) {                                  /* fills of 1,2,..,67 bytes, repeating */
            if (n + k > CHUNK) { if (fwrite(buf, 1, n, stdout) != n) return 0; n = 0; }
            pawl_fill(&g, buf + n, k); n += k;
            k = k % 67 + 1;
        }
    }
    fprintf(stderr, "unknown mode %s\n", mode);
    return 1;
}
