#!/bin/bash
# ./mRCQ.sh prefix {ticketing: yes|no}
args=("$@")
prefix=${args[0]}
ticket=${args[1]}

log=$PWD/${prefix}-rcq-t-${ticket}.log
echo ${log}
echo -ne "# " >> ${log}
date >> ${log}
for thread in 1 5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80
do 
    ../benchmark/mRCQRun --ticket ${ticket} --memaccess no --pinning yes --thread ${thread} >> ${log}
done
echo -ne "# " >> ${log}
date >> ${log}
