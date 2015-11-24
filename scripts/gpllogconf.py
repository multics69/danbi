#!/usr/bin/python
import sys
import os

# ./gplspeedup.py wxt RG.pdf.gpl r910-all-RecursiveGaussianRun.log RG-all r910-no-numa-RecursiveGaussianRun.log RG-no-numa r910-no-prs-RecursiveGaussianRun.log RG-no-prs r910-no-pss-RecursiveGaussianRun.log RG-no-pss r910-no-pss-no-prs-RecursiveGaussianRun.log RG-no-pss-no-prs r910-no-ticket-RecursiveGaussianRun.log RG-no-ticket

def print_usage():
	print 'DANBI plot relative speed up of all benchmarks with a configuration. Usage:\n'
	print '    gpllogconf.py [terminal-type] [gnuplot-output-file] [log-file-prefix] [configuration-name]\n'
		
def main():
	# check number of argument 
	if len(sys.argv) != 5:
		print_usage()
		return

	# get argument
	term = sys.argv[1]
	gpl = sys.argv[2]
	prefix = sys.argv[3]
	conf = sys.argv[4]

	# generate command line string
	benchmarks = ["RecursiveGaussian", "SRAD2", "FilterBank", "FMRadio", "MergeSort", "FFT-fusion", "TDE-fusion"]
	command = "./gplspeedup.py " + term + " " + gpl
	for i in range(len(benchmarks)):
		bench = benchmarks[i]
		command = command + " " + prefix + "-" + conf + "-" + bench + "Run.log " + bench + "-" + conf
	# run command 
	os.system(command)
	
	# for terminal type
	if term == "pdf":
		os.system("gnuplot " + gpl + " > " + gpl+".pdf")
	else:
		os.system("gnuplot -p " + gpl)

if __name__=="__main__":
	main()

