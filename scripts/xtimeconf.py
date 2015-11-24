#!/usr/bin/python
import sys
import os

def print_usage():
	print 'DANBI extract time from all benchmarks with a configuration. Usage:\n'
	print '    xtimeconf.py [log-file-prefix] [configuration-name] [output-file]\n'
		
def main():
	# check number of argument 
	if len(sys.argv) != 4:
		print_usage()
		return
	# get argument
	prefix  = sys.argv[1]
	conf    = sys.argv[2]
	outfile = sys.argv[3]

	# run for all bench 
	mergecmd = "./colmerge.py " + outfile 
	benchmarks = ["RecursiveGaussian", "SRAD2", "FilterBank", "FMRadio", "MergeSort", "FFT-fusion", "TDE-fusion"]
	for i in range(len(benchmarks)):
		bench = benchmarks[i]
		logname = prefix + "-" + conf + "-" + bench + "Run.log"
		timename = logname + ".time~"
		os.system("./xlog.py " + logname + " 3 " + bench + " " + timename)
		mergecmd = mergecmd + " " + timename
	
	# combine
	os.system(mergecmd)
	os.system("rm -f *.time~")

if __name__=="__main__":
	main()

