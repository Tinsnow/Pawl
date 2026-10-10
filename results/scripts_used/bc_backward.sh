#!/bin/bash
# Runs BigCrush tests one per process, from test 106 downwards, until it meets the
# sequential BigCrush process that is working upwards from test 1.
SEQLOG=/home/claude/rng/results/testu01_bigcrush_forward.txt
OUT=/home/claude/rng/results/bigcrush_tests
flush_seq() { pid=$(pgrep -f "tu01 big fwd" | head -1); [ -n "$pid" ] && gdb -p $pid -batch -ex "call (int)fflush(0)" > /dev/null 2>&1; }
for t in $(seq 106 -1 1); do
  flush_seq
  done_seq=$(grep -c 'CPU time used' $SEQLOG)
  echo "$(date +%H:%M:%S) about to run test $t; sequential run has completed $done_seq" >> $OUT/progress.txt
  if [ $t -le $((done_seq + 1)) ]; then
    # the sequential process is on test done_seq+1 == t (or beyond): let it finish that one, then stop it
    while true; do flush_seq; d=$(grep -c 'CPU time used' $SEQLOG); [ $d -ge $t ] && break; sleep 20; done
    flush_seq; pkill -f "tu01 big fwd"
    echo "$(date +%H:%M:%S) met at test $t; sequential run completed tests 1..$d" >> $OUT/progress.txt
    echo $d > $OUT/sequential_completed.txt
    break
  fi
  ./tu01_one one $t fwd "testu01 bigcrush test $t" > $OUT/test_$(printf %03d $t).txt 2>&1
done
echo finished >> $OUT/progress.txt
