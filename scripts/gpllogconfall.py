#!/usr/bin/python
import sys
import os

def print_usage():
	print 'DANBI plot relative speed up of all benchmark with all configurations per configuration. Usage:\n'
	print '    gpllogconfall.py [log-file-prefix]\n'
		
def main():
	# check number of argument 
	if len(sys.argv) != 2:
		print_usage()
		return

	# get argument
	prefix = sys.argv[1]

	# run for all bench 
	confs = ["RCQ", "RCQ-SCH-pss", "RCQ-SCH-NUMA"]
#	confs = ["baseline", "RCQ", "RCQ-SCH-pss", "RCQ-SCH-NUMA"]
#	confs = ["baseline", "RCQ-ticket-ordering", "RCQ-ticket-ordering-commit-ordering", "RCQ-SCH-pss", "RCQ-SCH-pss-prs", "RCQ-SCH-NUMA-interleave", "RCQ-SCH-NUMA"]
#	confs = ["all", "no-ssq-use-msq", "no-per-cpu-ticket-waiting", "no-queue-based-commit-ordering", "no-ticket", "no-numa", "no-conpf", "no-interleave", "no-pss-no-prs", "no-pss", "no-prs"]
	for i in range(len(confs)):
		conf = confs[i]
		gpl = prefix + "-" + conf + "-ALL.gpl"
		os.system("./gpllogconf.py pdf " +  gpl + " " + prefix + " " + conf)
		os.remove(gpl)

if __name__=="__main__":
	main()

