#!/bin/bash
args=("$@")
files=${args[0]}

#for bench in msqueue-nobackoff.c ccqueue.c hqueue.c lcrq.c hlcrq.c locksqueue.c msqueue.c simqueue.c lecdq lebdq
for bench in lecdq
do 
    for workload in p 5
    do
	for initelms in 0 256
	do
	    logall=./raw/${bench}-${workload}-${initelms}-*.log
	    logavg=${bench}-${workload}-${initelms}.log
	    ./avg.sh ${logall} > ${logavg} 
	done
    done
done

