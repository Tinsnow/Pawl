# Analysis tools

These programs chose Pawl's constants and attack its permutation with reduced round counts. `bash run_all.sh` rebuilds them and reruns the quick experiments; the slow ones are listed at the bottom of that script with their run times. `logs/` holds the output of the runs quoted in the main README.

All bit numbers are positions in the 512-bit state: word `w`, bit `k` is number `64*w + k`, with words 0 to 3 the `a` row and words 4 to 7 the `b` row.

| File | What it does |
|---|---|
| `perm.h` | The permutation and its inverse for any number of rounds, written separately from `pawl.c`. `xcheck.c` confirms the two agree. |
| `search.c` | First pass over round shapes: which column feeds which, whether the rows swap, whether the second rotation is worth its cost. |
| `tune2.c`, `run_tune.sh`, `final_eval.c` | Hill-climb over the eight rotation amounts for each of 24 column wirings, then high-precision rescoring of the finalists. |
| `aval.c` | Full 512 by 512 avalanche matrix: for every input bit, how often each output bit flips. |
| `dbias.c` | Differential bias. `all` ranks single-bit differences, `pairs` screens all 130,816 two-bit differences, `list` measures chosen differences to high precision. |
| `backdiff.c` | Finds the most likely input difference that turns into a wanted difference after one or two rounds. This is how the 7-round result was obtained. |
| `struct.c` | Rotational-pair test and cube sums (higher-order differentials). |
| `lin2.c` | Linear approximations: full Walsh spectrum of each output bit, and each adjacent pair of output bits, over 2^20-point input subcubes. |

## Reading the logs

- `constant_search_576_hillclimbs.txt`: one line per hill-climb, best first. The score is the worst-case diffusion described in the main README; `o=(1,1,2)` is the wiring Pawl uses and `o=(3,2,3)` is its mirror image.
- `constant_finalists.txt`: the candidates that passed the filter (eight distinct values, no multiples of 8, no 1 or 63), rescored with 1,500 samples. The top line is Pawl.
- In the differential logs, `chi z` pools all 512 output bits and behaves like a standard normal value when there is no bias; `max|z|` is the single most biased output bit, about 3 to 4 by chance alone.
- `differential_7_rounds_2bit.txt` includes one borderline value (`max|z| = 4.48` for `63+319`). The rerun with fresh inputs, in the same file, did not reproduce it.

## What the attacks found

| Rounds | Finding |
|---|---|
| 3 | Some output bits do not depend on some input bits. Cube sums vanish. Linear correlation 1. |
| 4 | Full dependence, but single-bit differences still give biases near 0.5. Linear correlation 2^-1.6. |
| 5 | Single-bit differential bias about 2^-4.5. Linear correlation 2^-6.4. PractRand fails the bare permutation. |
| 6 | Two-bit differences that cancel in the first addition (same bit of `a[i]` and `b[i]`) give bias up to 2^-7.9. Everything else is clean. |
| 7 | A five-bit difference that becomes one of those two-bit differences after one round, with probability 2^-5.4, gives bias 2^-13.1. Detected with 2^30 pairs. |
| 8 | Nothing found with 2^30 pairs. Going two rounds back costs probability 2^-18 (`differential_two_rounds_back.txt`), which predicts bias 2^-26 and about 2^54 pairs. |

Not tried, and worth trying by anyone who wants to go further: automated search for optimal multi-round differential and linear trails, differential-linear and boomerang combinations, inside-out distinguishers that start from the middle of the permutation, and algebraic attacks on the squeezing mode.
