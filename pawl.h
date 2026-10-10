/*
 * Pawl -- a ratcheting random number generator.  Version 1.
 *
 *   state   : 256-bit key + 128-bit block counter + 64-bit stream id
 *   core    : a 512-bit, 24-round add / rotate-xor / and permutation
 *   output  : 32 bytes per step; the key is replaced on every step, so
 *             no key is ever used for more than one permutation call and
 *             a captured state cannot be run backwards.
 *
 * EXPERIMENTAL.  This is a new design with no independent cryptanalysis.
 * It passes the statistical batteries it was tested against, which is not
 * the same as being secure.  Do not use it to protect real secrets; use
 * your operating system's generator for keys, tokens and passwords.
 *
 * Public domain / CC0.  Portable C99, no dependencies.
 */
#ifndef PAWL_H
#define PAWL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PAWL_ROUNDS
#define PAWL_ROUNDS 24          /* must be even; anything below 24 is for testing only */
#endif

typedef struct {
    uint64_t key[4];            /* secret, replaced on every step            */
    uint64_t ctr[2];            /* 128-bit block counter (guarantees period) */
    uint64_t stream;            /* stream id                                 */
    uint64_t buf[4];            /* unread output of the last step            */
    unsigned pos;               /* bytes of buf already handed out (0..32)   */
} pawl_t;

/* Seed from arbitrary bytes (any length, may be 0) and a stream id.
 * Equal (seed, stream) pairs give equal output; anything else gives an
 * unrelated sequence. */
void pawl_init(pawl_t *g, const void *seed, size_t len, uint64_t stream);

/* Convenience: seed from one 64-bit integer (encoded little-endian), stream 0. */
void pawl_init_u64(pawl_t *g, uint64_t seed);

/* Seed with 32 bytes from the operating system.  Returns 0 on success, -1 if
 * no entropy source could be read; the generator is then NOT seeded and must
 * not be used.  Always check the return value. */
int  pawl_init_os(pawl_t *g);

/* Mix fresh material into the key.  The old key cannot be recovered from the
 * new one, and the new one cannot be predicted without knowing `data`. */
void pawl_reseed(pawl_t *g, const void *data, size_t len);

/* Output.  All of these read the same underlying byte stream. */
void     pawl_fill(pawl_t *g, void *out, size_t len);
uint64_t pawl_u64(pawl_t *g);
uint32_t pawl_u32(pawl_t *g);
double   pawl_double(pawl_t *g);               /* uniform in [0,1), 53 bits   */
uint64_t pawl_below(pawl_t *g, uint64_t n);    /* uniform in [0,n), unbiased  */

/* Erase the generator. */
void pawl_wipe(pawl_t *g);

/* The bare permutation, exposed for testing and analysis. */
void pawl_permute(uint64_t s[8], int rounds);

#ifdef __cplusplus
}
#endif
#endif
