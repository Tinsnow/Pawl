#!/bin/bash
# $1 = which half (0/1)
i=0
for o2 in 0 1 2 3; do for pr in "0 1" "0 2" "0 3" "1 2" "1 3" "2 3"; do
  if [ $((i % 2)) -eq $1 ]; then ./tune2 $o2 $pr 24 $((1000+i)) 120; fi
  i=$((i+1))
done; done
