#!/usr/bin/python
import sys
import os

def print_usage():
	print 'DANBI plot relative speed up of all benchmark with all configurations per benchmark. Usage:\n'
	print '    gpllogbenchall.py [log-file-prefix]\n'
		
def main():
	# check number of argument 
	if len(sys.argv) != 2:
		print_usage()
		return

	# get argument
	prefix = sys.argv[1]

	# run for all bench 
	benchmarks = ["RecursiveGaussian", "SRAD2", "FilterBank", "FMRadio", "MergeSort", "FFT-fusion", "TDE-fusion"]
	for i in range(len(benchmarks)):
		bench = benchmarks[i]
		gpl = prefix + "-ALL-" + bench + ".gpl"
		os.system("./gpllogbench.py pdf " +  gpl + " " + prefix + " " + bench)
		os.remove(gpl)

if __name__=="__main__":
	main()

