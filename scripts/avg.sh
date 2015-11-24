#!/bin/bash
args=("$@")
files=${args[0]}

sort -n ${files} | grep -v -e "# " | awk -f avg.awk

