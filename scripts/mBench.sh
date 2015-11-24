#!/bin/bash
# ./mBench.sh bench prefix
args=("$@")
bench=${args[0]}
prefix=${args[1]}
perf="off"

log=$PWD/${prefix}-${bench}.log
../benchmark/bConfRun >> ${log}

if [ "$perf" = "off" ]; then
#for core in 1 5 10 15 20 25 30 35 40
#for core in 40
for core in 1 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80
do 
    eventlog=$PWD/${prefix}-${bench}-${core}
    echo ${log}
    echo -ne "# " >> ${log}
    date >> ${log}
    ../benchmark/${bench} --core ${core} --log ${eventlog} >> ${log}
    echo -ne "# " >> ${log}
    date >> ${log}
    rm *-Simple.gpl
done
else
#for core in 1 5 10 15 20 25 30 35 40
#for core in 40
for core in 1 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80
do 
    eventlog=$PWD/${prefix}-${bench}-${core}
    perfdata=$PWD/${prefix}-${bench}-${core}.perf.data
    echo ${log}
    echo -ne "# " >> ${log}
    date >> ${log}
    perf record -o ${perfdata} ../benchmark/${bench} --core ${core} --log ${eventlog} >> ${log}
    echo -ne "# " >> ${log}
    date >> ${log}
    rm *-Simple.gpl
done
fi

