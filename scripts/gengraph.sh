#!/bin/bash
VIEWER=evince
OS=`uname`
if [ $OS == "Darwin" ]
then 
    VIEWER=open
fi


args=("$@")
xaxis=${args[0]}
yaxis=${args[1]}
workload=${args[2]}
initelms=${args[3]}

labels=("thr" "mops" "comb" "atomic" "l1" "l2" "l3")
pdf_name=fig-${labels[$xaxis]}-${labels[$yaxis]}-${workload}-${initelms}.pdf

./gengpl.sh $xaxis $yaxis $workload $initelms | gnuplot > $pdf_name
$VIEWER  $pdf_name
