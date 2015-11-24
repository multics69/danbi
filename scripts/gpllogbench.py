#!/usr/bin/python
import sys
import os

# ./gplspeedup.py wxt RG.pdf.gpl r910-all-RecursiveGaussianRun.log RG-all r910-no-numa-RecursiveGaussianRun.log RG-no-numa r910-no-prs-RecursiveGaussianRun.log RG-no-prs r910-no-pss-RecursiveGaussianRun.log RG-no-pss r910-no-pss-no-prs-RecursiveGaussianRun.log RG-no-pss-no-prs r910-no-ticket-RecursiveGaussianRun.log RG-no-ticket

def print_usage():
	print 'DANBI plot relative speed up of a benchmark with all configurations. Usage:\n'
	print '    gpllogbench.py [terminal-type] [gnuplot-output-file] [log-file-prefix] [benchmark-name]\n'
		
def main():
	# check number of argument 
	if len(sys.argv) != 5:
		print_usage()
		return

	# get argument
	term = sys.argv[1]
	gpl = sys.argv[2]
	prefix = sys.argv[3]
	benchmark = sys.argv[4]

	# generate command line string
	confs = ["baseline", "RCQ-ticket-ordering", "RCQ-ticket-ordering-commit-ordering", "RCQ-SCH-pss", "RCQ-SCH-pss-prs", "RCQ-SCH-NUMA-interleave", "RCQ-SCH-NUMA"]
#	confs = ["all", "no-ssq-use-msq", "no-per-cpu-ticket-waiting", "no-queue-based-commit-ordering", "no-ticket", "no-numa", "no-conpf", "no-interleave", "no-pss-no-prs", "no-pss", "no-prs"]
	command = "./gplspeedup.py " + term + " " + gpl
	for i in range(len(confs)):
		conf = confs[i]
	        command = command + " " + prefix + "-" + conf + "-" + benchmark + "Run.log " + benchmark + "-" + conf 

	# run command 
	os.system(command)
	
	# for terminal type
	if term == "pdf":
		os.system("gnuplot " + gpl + " > " + gpl+".pdf")
	else:
		os.system("gnuplot -p " + gpl)

if __name__=="__main__":
	main()

