#!/bin/bash
args=("$@")
xaxis=${args[0]}
yaxis=${args[1]}
workload=${args[2]}
initelms=${args[3]}

q_bench_names=(lecdq lebdq hqueue.c ccqueue.c hlcrq.c lcrq.c msqueue.c msqueue-nobackoff.c locksqueue.c)
q_str_names=("LECD Queue" "LEBD Queue" "H-Queue" "CC-Queue" "LCRQ+H" "LCRQ" "MS-Queue (backoff)" "MS-Queue (no backoff)" "Two lock Queue")

if [ $yaxis -eq "2" ] 
then 
    q_bench_names=(lecdq hqueue.c ccqueue.c)
    q_str_names=("  LECD Queue" "  H-Queue" "  CC-Queue")
fi


axis_lables=("Threads" "M ops/second" "Average Combining" "Atomic Instructions/op" "L1 cache misses/op" "L2 cache misses/op" "L3 cache misses/op")
y_lower_bounds=(0 0 0 1 0 0 0)
y_upper_bounds=(80 35 250 8 "" "" "")

echo set terminal pdf color dashed size 17.5cm, 16.5cm font \"Arial,9\"
echo set ylabel \"${axis_lables[$yaxis]}\"
echo set xlabel \"${axis_lables[$xaxis]}\"
echo set key ins vert
echo set key left top

if [ $yaxis -eq "3" ] 
then 
    echo set key horiz
    echo set key top center
fi

echo set style line 1 lt 1 lw 7 pt 3 lc rgb \"#375D97\"
echo set style line 2 lt 1 lw 7 pt 4 lc rgb \"#983334\" 
echo set style line 3 lt 1 lw 7 pt 5 lc rgb \"#77973D\"
echo set style line 4 lt 1 lw 7 pt 6 lc rgb \"#5A407A\"
echo set style line 5 lt 1 lw 7 pt 7 lc rgb \"#32849E\"
echo set style line 6 lt 1 lw 7 pt 8 lc rgb \"#D1702F\" 
echo set style line 7 lt 1 lw 7 pt 9 lc rgb \"#7C93BF\" 
echo set style line 8 lt 1 lw 7 pt 10 lc rgb \"#7C93BF\" 
echo set style line 9 lt 1 lw 7 pt 11 lc rgb \"#88AE43\" 
echo set style line 10 lt 1 lw 7 pt 12 lc rgb \"#694A8E\" 
echo set style line 11 lt 1 lw 7 pt 13 lc rgb \"#3A9AB9\" 
echo set style line 12 lt 1 lw 7 pt 14 lc rgb \"#F17F32\" 
echo set style line 13 lt 1 lw 7 pt 15 lc rgb \"#95A7C9\" 
echo set style line 14 lt 1 lw 7 pt 16 lc rgb \"#CA9394\" 
echo set style line 15 lt 1 lw 7 pt 17 lc rgb \"#B4C997\" 
echo set style line 16 lt 1 lw 7 pt 18 lc rgb \"#A79AB8\" 
echo set style line 17 lt 1 lw 7 pt 19 lc rgb \"#9BC3D2\" 

echo -n plot [0:80][${y_lower_bounds[$yaxis]}:${y_upper_bounds[$yaxis]}]
count=0
for i in "${q_bench_names[@]}"
do 
    log=$i-${workload}-${initelms}.log
    count=$(($count+1))
    if [ "$count" -ne "1" ]
    then 
	echo -n ", "
    fi
    echo -n " \"$log\" using (\$$(($xaxis+1))):(\$$(($yaxis+1))) w lp ls $count title \"${q_str_names[$(($count-1))]}\""
done
echo 

