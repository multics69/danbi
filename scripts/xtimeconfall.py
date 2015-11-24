#!/usr/bin/python
import sys
import os

def print_usage():
	print 'DANBI extract time from all benchmarks with all configurations. Usage:\n'
	print '    xtimeconfall.py [log-file-prefix]\n'
		
def main():
	# check number of argument 
	if len(sys.argv) != 2:
		print_usage()
		return

	# get argument
	prefix = sys.argv[1]

	# run for all bench 
	confs = ["baseline", "RCQ-ticket-ordering", "RCQ-ticket-ordering-commit-ordering", "RCQ-SCH-pss", "RCQ-SCH-pss-prs", "RCQ-SCH-NUMA-interleave", "RCQ-SCH-NUMA"]
#	confs = ["all", "no-ssq-use-msq", "no-per-cpu-ticket-waiting", "no-queue-based-commit-ordering", "no-ticket", "no-numa", "no-conpf", "no-interleave", "no-pss-no-prs", "no-pss", "no-prs"]
	for i in range(len(confs)):
		conf = confs[i]
		output = prefix + "-" + conf + "-time.csv"
		os.system("./xtimeconf.py " +  prefix + " " + conf + " " + output);

if __name__=="__main__":
	main()

