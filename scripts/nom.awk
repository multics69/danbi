{
    if ($1 == "#") {
	print
    }
    else {
	for(i = 1; i <= 3; i++)
	    printf("%s ", $i)
	for(i = 4; i <= 7; i++)
	    printf("%s ", $i/2)
	printf("\n")
    }
}
