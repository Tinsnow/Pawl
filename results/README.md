# Raw test logs

Everything quoted in the main README, unedited.

| File | What it is |
|---|---|
| `practrand_pawl24_256GB.txt` | Main PractRand run on the full generator (`pawl_stream gen 'practrand main run'`), stopped by hand after the 256 GB checkpoint. |
| `practrand_seeds_0_1_2.txt` | First 32 bytes from each of the integer seeds 0, 1, 2, ... concatenated. |
| `practrand_64_streams_interleaved.txt` | 64 streams of one seed, 8 bytes from each in turn. |
| `practrand_generator_1_round.txt`, `..._2_rounds.txt` | The generator built with a 1-round and a 2-round permutation. |
| `practrand_bare_permutation_N_rounds.txt` | The N-round permutation applied to a plain counter, no key, all 512 output bits. |
| `testu01_smallcrush.txt` | SmallCrush, complete. |
| `testu01_bigcrush_summary.txt` | One table of every BigCrush p-value, built by `make_bigcrush_summary.py` from the logs below. |
| `testu01_bigcrush_tests_001_043.txt` | BigCrush tests 1 to 43, run in order in one process. |
| `bigcrush_tests/test_044.txt` to `test_106.txt` | BigCrush tests 44 to 106, one process per test, each on its own stream. |
| `bigcrush_test45_reruns/` | Six reruns of test 45 on fresh streams. |
| `selftest.txt`, `benchmark.txt` | Output of `tools/selftest` and `tools/bench`. |
| `scripts_used/` | The shell scripts that drove these runs, as they were (they contain paths from the test machine). |

## How BigCrush was run

BigCrush took far longer than expected on the two-core test machine, so it was split. One process started the battery normally and worked upward from test 1. A second worker ran single tests from 106 downward, each in its own process and seeded with its own text (`testu01 bigcrush test N`). The two met at test 43, and the first process was stopped there, which is why its log ends without TestU01's usual summary and with the start of a test it never finished. Every one of the 106 tests ran exactly once to completion, on the 32-bit outputs in the order the generator produces them.

Splitting changes which part of the output each test sees. It does not change the tests or their parameters.

Not run: the bit-reversed variant of BigCrush, and PractRand beyond 256 GB.
