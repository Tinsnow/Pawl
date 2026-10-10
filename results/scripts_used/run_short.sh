#!/bin/bash
PR=/home/claude/rng/third_party/practrand/RNG_test
OUT=/home/claude/rng/results
while pgrep -x lin2 > /dev/null; do sleep 5; done
./ps24 seeds        | $PR stdin64 -tlmin 1GB -tlmax 8GB  > $OUT/practrand_seeds_0_1_2.txt 2>&1
./ps24 streams 64   | $PR stdin64 -tlmin 1GB -tlmax 8GB  > $OUT/practrand_64_streams_interleaved.txt 2>&1
./ps6 ctr           | $PR stdin64 -tlmin 1GB -tlmax 16GB > $OUT/practrand_bare_permutation_6_rounds.txt 2>&1
./ps7 ctr           | $PR stdin64 -tlmin 1GB -tlmax 16GB > $OUT/practrand_bare_permutation_7_rounds.txt 2>&1
./ps2 gen           | $PR stdin64 -tlmin 1GB -tlmax 16GB > $OUT/practrand_generator_2_rounds.txt 2>&1
for r in 3 4 5; do ./ps$r ctr | $PR stdin64 -tlmax 1GB > $OUT/practrand_bare_permutation_${r}_rounds.txt 2>&1; done
./ps1 gen           | $PR stdin64 -tlmax 1GB > $OUT/practrand_generator_1_round.txt 2>&1
echo done > $OUT/.short_done
