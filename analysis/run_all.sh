#!/bin/bash
# Rebuilds every analysis tool and reruns the quick experiments (a few minutes on one core).
# The long ones are listed at the bottom, commented out, with their run times.
set -e
CC="${CC:-gcc} -O3 -march=native"
$CC -o aval aval.c -lm
$CC -fopenmp -o dbias dbias.c -lm
$CC -fopenmp -o struct struct.c -lm
$CC -o lin2 lin2.c -lm
$CC -o backdiff backdiff.c -lm
$CC -o tune2 tune2.c -lm
$CC -o final_eval final_eval.c -lm
$CC -o search search.c -lm
$CC -o xcheck xcheck.c ../pawl.c -I.. -lm
mkdir -p logs
./xcheck                                              | tee logs/library_matches_analysis_code.txt
(./aval 1 8 8192 0; ./aval 1 8 8192 1)                | tee logs/avalanche_rounds_1_to_8.txt
./struct rot 20000                                    | tee logs/rotational.txt
(for r in 2 3 4 5; do for w in lowb lowa high spread rand; do ./struct cube $r 16 12 $w; done; done
 for w in lowb spread rand; do ./struct cube 4 24 10 $w; done) | tee logs/cube_dim16_24.txt
(./dbias pairs 5 64 0; ./dbias pairs 5 64 1)          | tee logs/differential_slowest_2bit_pairs.txt
(./dbias list 6 67108864 0 77+333 48+304 127+383 67+323 63+319 255+511 191+447 218+474
 ./dbias list 6 67108864 1 83+454 152+160 26+241 168+186) | tee logs/differential_6_rounds_2bit.txt
./backdiff 2000000 3 1 218+474 77+333 127+383 63+319 255+511 191+447 | tee logs/differential_one_round_back.txt
./backdiff 16777216 3 2 218+474 77+333                             | tee logs/differential_two_rounds_back.txt   # needs 1 GB of memory

# Longer runs (times are for one core of a 2.8 GHz Xeon):
#   ./run_tune.sh 0 > h0.txt & ./run_tune.sh 1 > h1.txt      constant search, 576 hill-climbs, ~8 min
#   ./struct cube 5 28 8 rand ; ./struct cube 6 30 6 lowb     big cubes, 5-15 min each
#   for r in 3 4 5 6 7 8; do ./lin2 $r 20 3 rand; ./lin2 $r 20 3 low; done     ~15 min
#   ./dbias list 7 1073741824 0 218+474 77+333 127+383 63+319                  ~20 min
#   ./dbias list 7 1073741824 0 156+214+245+412+501 29+91+111+285+347          ~15 min  (the 7-round break)
#   ./dbias list 8 1073741824 0 156+214+245+412+501 29+91+111+285+347          ~15 min
