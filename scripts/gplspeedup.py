#!/usr/bin/python
import sys

def gplspeedup(term, gpl):
	# open output file
	gf = open(gpl, 'w')

	# print header
	if term == "pdf":
		gf.write("set terminal pdf size 19.8cm, 18cm font \"Arial,14\"\n")
	else:
		gf.write("set terminal " + term + " size 550, 500 font \"Arial,14\"\n")
	gf.write("set key left top\n")
	gf.write("set ylabel \"relative speedup\"\n")
	gf.write("set xlabel \"number of processes\"\n")
	gf.write("plot [0:80][0:] x with dots title \"\"")

	# for each log
	for i in range(3, len(sys.argv), 2):
		# open log file and get the associated title name
		logfile = sys.argv[i]
		lf = open(logfile, 'r')
		title = sys.argv[i+1]

		# for each line of the log file
		singlecore = 1
		ll = lf.readlines()
		for li in range(len(ll)):
			# skip comment line 
			if ll[li][0] == '#':
				continue
			# split a line
			sll = ll[li].split()
			# for the single core result 
			if sll[0] != '1':
				continue
			singlecore = sll[2]
			break
		# print for a log
		if singlecore != 1:
			gf.write(", \"" + logfile + "\" using ($1):(" + singlecore + "/$3) w lp title \"" + title + "\"")
		lf.close()

	# close output file 
	gf.write("\n")
	gf.close()

def print_usage():
	print 'DANBI performance log to speedup in gnuplot format. Usage:\n'
	print '    gplspeedup.py [terminal-type] [gnuplot-output-file] {[performance-log] [title]}*\n'
		
def main():
	# check number of argument 
	if len(sys.argv) < 5 or len(sys.argv)%2 == 0:
		print_usage()
		return

	# get argument
	term = sys.argv[1]
	gpl = sys.argv[2]
	gplspeedup(term, gpl)

if __name__=="__main__":
	main()

