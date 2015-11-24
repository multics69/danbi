BEGIN { 
    printf("# threads(1) MOpsPerSec(2) AvgCombining(3) AtomicPerOp(4)  L1D$Miss(5) L2D$Miss(6) L3$Miss(7)\n") 
    for(i = 1; i <= 7; i++)
	value[i] = 0
    count = 0
} 
{
    if ($1 != value[1]) {
	if (count > 0) {
	    printf("%s ", value[1])
	    for(i = 2; i <= 7; i++)
		printf("%s ", value[i]/count)
	    printf("\n")
	}
	value[1] = $1
	for(i = 2; i <= 7; i++)
	    value[i] = 0
	count = 0
    }
    count++
    for(i = 2; i <= 7; i++)
	value[i] += $i
}
END {
    if (count > 0) {
	printf("%s ", value[1])
	for(i = 2; i <= 7; i++)
	    printf("%s ", value[i]/count)
	printf("\n")
    }
}
