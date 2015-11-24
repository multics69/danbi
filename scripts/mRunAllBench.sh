#!/bin/bash
# ./mRunAllBench.sh prefix
args=("$@")
prefix=${args[0]}


#for core in {1..10..1}
for core in {1..1..1}
do 
     sync
     sleep 1
     ./mBench.sh SRAD2Run  ${prefix}
     sync
     sleep 1
     ./mBench.sh RecursiveGaussianRun ${prefix}
     sync
     sleep 1
     ./mBench.sh FFT-fusionRun  ${prefix}
     sync
     sleep 1
     ./mBench.sh TDE-fusionRun ${prefix}
     sync
     sleep 1
     ./mBench.sh FilterBankRun ${prefix}
     sync
     sleep 1
     ./mBench.sh MergeSortRun ${prefix}
     sync
     sleep 1
     ./mBench.sh FMRadioRun ${prefix}
done    

