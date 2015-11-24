#!/bin/bash
# ./cntevent.sh event.gpl
args=("$@")
eventlog=${args[0]}

echo "# Total number of scheduling"
cat ${eventlog} | grep "OE " | wc -l 

echo "# Number of scheduling caused by waiting"
cat ${eventlog} | grep "OE t" | wc -l 

