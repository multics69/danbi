#!/bin/bash
# ./mSSQPS.sh prefix {running_seconds}
args=("$@")
prefix=${args[0]}
seconds=${args[1]}

for initelms in 0 1000
do 
    log=$PWD/${prefix}-ssqps-s-${seconds}-i-${initelms}.log
    echo ${log}
    echo -ne "# " >> ${log}
    date >> ${log}
    for thread in 1 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80
    do 
	../benchmark/mSSQPSRun --pinning yes --runningsec ${seconds} --initelms ${initelms} --thread ${thread} >> ${log}
    done
    echo -ne "# " >> ${log}
    date >> ${log}
done
