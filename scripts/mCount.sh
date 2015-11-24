#!/bin/bash
# ./mBench.sh log out
args=("$@")
log=${args[0]}
out=${args[1]}

for core in {101..1000001..100}
do 
    cat $log | grep " $core" | wc -l >> $out
done
