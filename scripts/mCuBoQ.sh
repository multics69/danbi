#!/bin/bash
# ./mCuBoQ.sh 
args=("$@")

sync
sleep 5
for suffix in {1..5}
do
    for workload in p 5
    do
	for initelms in 0 256
	do 
	    logname=lecdq-wc-${workload}-${initelms}-----${suffix}.log
	    rm ${logname}
	    for thread in 1 4 8 16 24 32 40 48 56 64 72 80
	    do 
		sync
		sleep 1
		echo ../benchmark/mCuBoQ2Run --initelms ${initelms} --thread ${thread} --type ${workload}
		../benchmark/mCuBoQ2Run --initelms ${initelms} --thread ${thread} --type ${workload} >> ${logname}
	    done
	done
    done
done
