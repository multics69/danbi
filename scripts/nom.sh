#!/bin/bash
args=("$@")
infile=${args[0]}
outfile=${args[1]}

awk -f nom.awk ${infile} > ${outfile}

