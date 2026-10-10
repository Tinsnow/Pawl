# Pawl

A ratcheting random number generator. Version 1, experimental.

A pawl is the small catch that lets a ratchet wheel turn one way and never back. This generator works the same way: every 32 bytes of output it replaces its own secret key with a new one that cannot be turned back into the old one.

| | |
|---|---|
| Secret state | 256-bit key |
| Other state | 128-bit step counter, 64-bit stream number |
| Core | one 512-bit permutation, 24 rounds of add, rotate, xor and AND on 64-bit words |
| Output | 32 bytes per step; the key is replaced on every step |
| Seed | any byte string, plus a stream number |
| Code | portable C99 (`pawl.c`, `pawl.h`) and a Python reference (`pawl.py`); no dependencies |

## Read this first

Pawl was designed by an AI. Claude, a model made by Anthropic, produced the design, the code, the tests, the attacks and this document in one session in October 2026, in response to a single prompt asking for a new random number generator. No human cryptographer has reviewed any of it. It has had a few hours of analysis, all of it by its designer. Two things follow.

- **What the evidence supports.** Its output showed no flaw in any statistical test it was given (listed below), including TestU01 BigCrush and PractRand to 256 GB. One BigCrush value fell just outside the pass band and did not repeat in six reruns. The best attack found on its core breaks 7 of the 24 rounds, and only in a setting far more generous to the attacker than the generator allows.
- **What the evidence does not support.** Passing statistical tests does not show that a generator is unpredictable; a 2-round version of Pawl also passes PractRand, and nobody should trust that. Generators such as ChaCha20 and AES-CTR have survived decades of public attack. Pawl has not been attacked by anyone but its author, so it cannot be called more secure than they are, and it should not be used for keys, tokens, passwords or anything else that protects real secrets. Use your operating system's generator for those.

It is a reasonable choice for simulations, games, procedural content, shuffling, sampling and tests, where you want output with no detectable pattern and reproducible streams. It is also a complete, documented design for anyone who wants to study it or try to break it.

## Using it

C:

```c
#include "pawl.h"

pawl_t g;
pawl_init_u64(&g, 12345);            /* reproducible: same seed, same output  */
/* or: pawl_init_os(&g);                unpredictable: 32 bytes from the OS   */

uint64_t a = pawl_u64(&g);           /* 64 random bits                        */
double   x = pawl_double(&g);        /* uniform in [0, 1)                     */
uint64_t d = 1 + pawl_below(&g, 6);  /* a fair die: no modulo bias            */
unsigned char buf[100];
pawl_fill(&g, buf, sizeof buf);      /* any number of bytes                   */
```

Build with any C99 compiler: `cc -O2 -c pawl.c`.

Python (slow, meant for checking other implementations):

```python
from pawl import Pawl
g = Pawl(12345)            # or Pawl(b"any bytes", stream=7)
g.u64(); g.random(); g.below(6); g.bytes(100)
```

Independent streams: give each worker the same seed and a different stream number, `pawl_init(&g, seed, len, worker_id)`. There is no jump-ahead function, on purpose: the generator cannot skip forward without doing the work, for the same reason it cannot go backward.

A 64-bit integer seed can be guessed by trying all 2^64 values. Where unpredictability matters, seed with at least 16 random bytes (`pawl_init_os` uses 32).

## The algorithm

All words are 64 bits. `+` is addition modulo 2^64, `^` is XOR, `&` is AND, `rotl(x, n)` rotates left by n bits. Bytes map to words little-endian.

### The permutation

The state is eight words in two rows, `a[0..3]` and `b[0..3]`. One round, with all indices taken modulo 4:

```
a[0] ^= (r + 1) * 0x9E3779B97F4A7C15          (r = round number from 0, product mod 2^64)
a[i]  = a[i] + b[i]                           for i = 0..3
b[i]  = rotl(b[i], R[i]) ^ a[i+1]             for i = 0..3
a[i]  = rotl(a[i], Q[i]) + (b[i+1] & b[i+2])  for i = 0..3
swap the two rows

R = (18, 50, 58, 37)      Q = (34, 30, 54, 4)
```

Each line uses the values produced by the line above it. The permutation `P` is 24 such rounds. Every step is reversible, so `P` is a bijection on 512-bit states; `pawl.py` includes the inverse and checks it.

### One step of the generator

The generator holds a key `K[0..3]`, a 128-bit counter `C` (two words, low word first) and a stream number `S`.

```
(a, b) = P( a = K,  b = (C_low, C_high, S, TAG_GEN) )
output = (b[0] + K[0], b[1] + K[1], b[2] + K[2], b[3] + K[3])     32 bytes, little-endian
K      = (a[0] ^ K[0], a[1] ^ K[1], a[2] ^ K[2], a[3] ^ K[3])
C      = C + 1
```

### Seeding and reseeding

Both use the permutation as a sponge. `absorb(tag, data)`:

```
a = K;   b = (length of data in bytes, 0, S, tag)
for each 32-byte block m of data (the last one padded with zero bytes):
    a = a ^ m;   (a, b) = P(a, b)
b[3] ^= TAG_END;   (a, b) = P(a, b)
K = a                                   (b is thrown away)
```

- **Seeding** sets `K = 0`, `C = 0`, stores `S`, then runs `absorb(TAG_KEY, seed)`.
- **Reseeding** runs `absorb(TAG_MIX, data)` on the current key, leaves `C` alone, and discards any bytes of the previous step that were not yet read.

The four tags are the ASCII strings `Pawl1gen`, `Pawl1key`, `Pawl1mix` and `Pawl1end` read as little-endian 64-bit integers.

### Derived outputs

All outputs read the same byte stream. `u64` is the next 8 bytes little-endian, `u32` the next 4. `double` is `(u64 >> 11) / 2^53`. `below(n)` draws a `u64`, keeps only as many low bits as `n - 1` needs, and redraws until the value is below `n`.

### Test vectors

```
P(0,0,0,0,0,0,0,0) =
  5a831e59c147c265 cbb617d4b6114eb2 b7e1d1888b756640 2e9d56d1e9d8e485
  ee5d837ddf1d06c3 a9018f8f202b4db3 0655d02495767c66 ca56b3628515bfbd

empty seed, stream 0, first 32 bytes:
  769d97478473f6d2344b3feae8a35a28adaa37746bf82321e52cd2fff88b283a

seed = integer 42 (8 bytes little-endian), stream 0:
  first 5 bytes aa65dd87e6, then u64 = 0f86f4cd2c57420b, then u32 = 8ad65303
```

`vectors.txt` has 93 in all, covering every seed length from 1 to 70 bytes, streams, reseeding, reads that straddle steps, and bytes 1,000,000 onward of a long run. The C library and the Python reference were written separately and agree on all of them (`make check`).

## Why it is built this way

**Every key is used once.** Most generators built from a cipher keep one key for a long stretch and feed it a counter, so an attacker sees many outputs of the same keyed function on closely related inputs. That is exactly the material the best known attacks on reduced-round ciphers need. Pawl calls its permutation once per key and then replaces the key, so no two calls ever share one. The cost is speed: half of what each call produces becomes the next key instead of output.

**A captured state cannot be run backward.** The next key is `a ^ K`, and `a` is never output. Someone who steals the state after a step holds the new key and may have seen the 32 output bytes, but to undo the step they need the old key, which is the thing they are trying to find. Output bytes are also erased from the buffer as they are handed out.

**Nothing leaves the permutation unmasked.** The output is `b + K`, not `b`. An observer never sees a raw output word of the permutation, only words blinded by a key that changes every step. The two halves are masked with different operations (addition for the output, XOR for the next key) so they cannot be cancelled against each other.

**The round mixes three kinds of arithmetic.** Addition is strongly non-linear in the high bits of a word and perfectly linear in the lowest bit. AND is equally non-linear at every bit position but moves nothing between positions. Rotation and XOR spread bits around but are linear. Each one covers a weakness of the others, which is the same reasoning behind SHA-2's mix of additions and Boolean functions. In Pawl each column of the state gets two additions, one AND and two rotations per round, and every operation takes the same time whatever the data is: there are no table lookups and no data-dependent branches.

**The counter rules out short cycles.** The counter is part of the state and goes up by one each step, so the state cannot repeat within 2^128 steps (2^133 bytes), whatever the key does.

**The constants were found by search.** The round shape and the eight rotation amounts came out of a search over 24 ways of wiring the columns together and several hundred hill-climbs over rotation sets. The score was worst-case diffusion: the smallest average number of output bits changed, after 3, 4 and 5 rounds, over every single-bit input change (forwards and backwards) and over the two-bit changes that cancel in the first addition. Sets with repeated values, multiples of 8, or rotations by 1 or 63 were dropped, and the best survivor at high precision was kept. The round constants are multiples of the 64-bit golden-ratio constant; they stop the rounds from being identical and break the symmetry between bit positions. `analysis/` contains the search and can regenerate the choice.

**24 rounds is about three and a half times the deepest attack found.** See the next section.

## What was tested

### How many rounds each attack breaks

These tests attack the permutation on its own, with the attacker choosing inputs freely and seeing all 512 output bits. The generator never allows that; it is the harshest setting available.

| Attack | Rounds broken | Detail |
|---|---|---|
| Incomplete diffusion | 3 | From 4 rounds, every output bit depends on every input bit. Flipping one input bit flips each output bit half the time from 6 rounds, to within the noise of 8,192 samples per bit pair. |
| Rotational symmetry | 2 | Gone by round 3 (round 5 if the round constants are removed). |
| Cube sums (higher-order differentials), cubes of 16 to 30 dimensions | 3 | No zero sums beyond chance at 4, 5 or 6 rounds. |
| PractRand on the permutation of a plain counter, no key | 5 | 3, 4 and 5 rounds fail within 1 GB. 6 and 7 rounds pass 16 GB. |
| Linear approximations (full Walsh spectra on 2^20-point subcubes) | 5 | Correlation 1 at 3 rounds, 2^-1.6 at 4, 2^-6.4 at 5; nothing above the noise floor of 2^-7.3 at 6, 7 or 8. |
| Differential bias, best two-bit input difference | 6 | Bias 2^-8 at 6 rounds. Nothing at 7 with 2^30 pairs. |
| Differential bias, five-bit difference built to produce that two-bit difference after one round | **7** | Bias 2^-13 at 7 rounds, found with 2^30 chosen pairs. The same differences show nothing at 8 rounds with 2^30 pairs. |

The last row is the deepest break found. The same method reaches 8 rounds only on paper: the best ten-bit difference found turns into the two-bit one after two rounds with probability 2^-18, which predicts a bias near 2^-26 and a need for roughly 2^54 chosen pairs, far more than could be run here. (The same arithmetic predicted the 7-round bias as 2^-13.3; the measured value was 2^-13.1.) A professional cryptanalyst should be expected to do better than this. The 24 rounds leave a factor of about 3.4 over the 7-round result. For comparison, after seventeen years of public work the best published attacks on ChaCha reach 7.5 of its 20 rounds [3].

For the generator itself, as opposed to the bare permutation, the ratchet hides a great deal: with a 1-round permutation it fails PractRand at once, and with only 2 rounds it already passes 16 GB. That is a warning about statistical tests, not a recommendation.

### Statistical tests on the full generator

| Test | Result |
|---|---|
| PractRand 0.94, default test set, 64-bit folding | no failures through 256 GB (2^38 bytes); the run was stopped there |
| TestU01 SmallCrush | all 15 statistics passed |
| TestU01 BigCrush | all 106 tests run, 160 statistics. 159 fall inside TestU01's pass band of 0.001 to 0.999. One, CollisionPermut with r = 10, came out at 0.9995. Six reruns of that test on fresh streams gave 0.74, 0.50, 0.51, 0.38, 0.18 and 0.57, so the first value was chance; with 160 statistics about one battery in four produces a value that far out. |
| PractRand on the first 32 bytes from each of the seeds 0, 1, 2, ... | no failures through 8 GB (2^28 seeds) |
| PractRand on 64 streams of one seed, interleaved | no failures through 8 GB |
| Seed avalanche: flip one bit of a 32-byte seed | 255.96 of 512 output bits change on average (ideal 256) |
| `below(n)` for n = 3, 6, 10, 1000, 65537; `double` | chi-square within 1 standard deviation of expected |

Across the three PractRand runs on the full generator there were 4,924 test results. Two were flagged "unusual", PractRand's mildest flag (p = 0.0008 at 8 GB in the main run, p = 0.996 at 4 GB in the seeds run). Each appeared at one checkpoint and was gone at the next, which is what chance produces at that rate. None was flagged "suspicious" or worse.

### Speed

Single thread on a 2.8 GHz Intel Xeon cloud machine, gcc 13.3 with `-O2`, median of five runs (`tools/bench.c`, raw numbers in `results/benchmark.txt`):

| Generator | MB/s | ns per 64-bit value | Unpredictable by design |
|---|---|---|---|
| xoshiro256++ | 7,200 | 1.1 | no |
| ChaCha20, OpenSSL 3.0's vector assembly | 3,900 | 2.1 | yes |
| Mersenne Twister (MT19937-64) | 3,000 | 2.7 | no |
| **Pawl** | **450** | **18** | yes |
| ChaCha20, plain C | 420 | 19 | yes |

Pawl runs at the speed of a plain C ChaCha20. It is about nine times slower than a ChaCha20 written with vector instructions, because ChaCha can compute many blocks side by side and Pawl cannot: each step needs the key the previous step produced. It is sixteen times slower than xoshiro256++, which makes no attempt to be unpredictable.

Compile with `-O2`. At `-O3 -march=native` some compilers try to vectorise the four columns of the round and the result is slower (clang 18: 275 MB/s).

## How it compares

| | Mersenne Twister | xoshiro256++ / PCG | ChaCha20-based generators | Pawl |
|---|---|---|---|---|
| Passes BigCrush and PractRand | no (fails linear-complexity tests) | yes | yes | yes, as far as tested |
| State recoverable from output | yes, from 624 outputs [4] | yes; not built to resist it | no known way | no way found |
| Old output safe if state leaks | no | no | back to the last key change, if the implementation erases keys | back to the last 32-byte step |
| Public cryptanalysis | not built to resist it | not built to resist it | since 2008, extensive | none |
| Speed | fast | very fast | fast with SIMD | moderate |

The honest summary: Pawl is far harder to predict than the fast statistical generators, and it erases its past more often than the usual ChaCha20 constructions do. On unpredictability it cannot be ranked above the established cryptographic generators, because that ranking is earned by years of failed attacks and Pawl has had none. On statistical quality there is no ranking to make: at these sample sizes none of the last three columns can be told apart from true randomness.

## Try to break it

That is what this repository is for. Some concrete targets, smallest first:

- **A mistake in the analysis.** Every number above comes from a log in `results/` or `analysis/logs/`, and `analysis/run_all.sh` reruns the quick experiments. A wrong claim is a finding.
- **Eight rounds of the permutation.** The existing differential method reaches 7 rounds and is estimated to need about 2^54 chosen pairs for 8. Anything that distinguishes 8 rounds from a random permutation with less work beats the designer. `analysis/dbias.c` measures a chosen difference and `analysis/backdiff.c` extends one backwards.
- **The generator with few rounds.** Build with `-DPAWL_ROUNDS=2`, `3` or `4`. The 2-round version passes PractRand to 16 GB here, and it should not survive anyone who looks at its structure.
- **The full generator.** A bias in the output, a way to recover the key, or a way to run a captured state backwards in fewer than 2^256 steps.
- **The code.** A disagreement between the specification, `pawl.c` and `pawl.py`, undefined behaviour, or a platform where the test vectors fail.

Open an issue with what you found. Findings will be credited here.

## Limits

- No independent review. Treat every security property above as a design goal.
- No jump-ahead and no random access.
- Copying the state (a forked process, a restored virtual-machine snapshot) produces two generators with identical output. Reseed after a fork.
- `pawl_init_os` reads `/dev/urandom`, or `BCryptGenRandom` on Windows. The Windows path has not been compiled or tested.
- Tested on little-endian x86-64 only. The code does not depend on byte order, but big-endian machines are untested.

## Files

| | |
|---|---|
| `pawl.h`, `pawl.c` | the library |
| `pawl.py` | Python reference, written separately from the C code |
| `vectors.txt`, `test_vectors.c`, `compare_vectors.py` | test vectors and the cross-check between the two implementations |
| `tools/` | byte-stream tool for test batteries, TestU01 driver, self-test, benchmark |
| `analysis/` | the constant search and the attacks on the reduced-round permutation |
| `results/` | raw logs of every test quoted above |
| `Makefile` | `make check` runs the cross-check and self-test |

## References

1. PractRand 0.94, Chris Doty-Humphrey. Built from the mirror at https://github.com/MartyMacGyver/PractRand
2. TestU01 1.2.3, Pierre L'Ecuyer and Richard Simard. Built from https://github.com/umontreal-simul/TestU01-2009
3. Sharma, Dey, Sarkar and Maitra, "On Improved Cryptanalytic Results against ChaCha for Reduced Rounds >= 7", https://eprint.iacr.org/2025/428
4. M. E. O'Neill, "Specific Problems with Other RNGs", https://www.pcg-random.org/other-rngs.html

Public domain (CC0).
