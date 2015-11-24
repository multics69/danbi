#!/usr/bin/python
import sys
import os

def print_usage():
	print 'DANBI run all benchmark scenario by adding optimization techniques one by one. Usage:\n'
	print '    mRunAllConfByAdd.py [prefix]\n'
		
def main():
	# check number of argument 
	if len(sys.argv) != 2:
		print_usage()
		return
	prefix = sys.argv[1]
	
	# define configuration array
	confs = { \
#		"baseline": \
#			"-DDANBI_DISABLE_BLOCKING_RESERVE_COMMIT_QUEUE -DDANBI_DISABLE_SPECULATIVE_SCHEDULING -DDANBI_DISABLE_RANDOM_SCHEDULING", \
#		"RCQ": \
#			"-DDANBI_DISABLE_SPECULATIVE_SCHEDULING -DDANBI_DISABLE_RANDOM_SCHEDULING", \
#		"RCQ-SCH-pss": \
#			"-DDANBI_DISABLE_RANDOM_SCHEDULING", 
	        "RCQ-SCH-NUMA": \
			""\
	}

	# run for all configurtions
	for i in confs.keys():
		cmd = "cd .. && "
		cmd = cmd + "cmake -DCMAKE_BUILD_TYPE=release -DDANBI_CONF=\"" + \
		    confs[i] + "\" . && "
		cmd = cmd + "make clean && "
		cmd = cmd + "make -j40 && "
		cmd = cmd + "cd scripts && "
		cmd = cmd + "./mRunAllBench.sh " + prefix + "-" + i
		os.system(cmd)

if __name__=="__main__":
	main()

