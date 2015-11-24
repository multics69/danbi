#!/usr/bin/python
import sys

def colmerge(outfile, sidx, eidx):
	# open file and read lines
	outf = open(outfile, 'w')
	inf=[0]*eidx
	infl=[0]*eidx
	for fi in range(sidx, eidx, 1):
		inf[fi] = open(sys.argv[fi])
		infl[fi] = inf[fi].readlines()

	# extract column from the input file 
	for li in range(len(infl[sidx])):
		for fi in range(sidx, eidx, 1):
			outf.write(infl[fi][li].rstrip('\r\n'))
			if fi != (eidx-1):
				outf.write(", ")
		outf.write("\n")
		
	# close file
	for fi in range(sidx, eidx, 1):
		inf[fi].close()
	outf.close()

def print_usage():
	print 'DANBI column-wise merge of multiple logs to one log. Usage:\n'
	print '   colmerge.py [output-file] {input-file}*\n'
		
def main():
	# check number of argument 
	if len(sys.argv) < 2:
		print_usage()
		return

	# get argument
	outfile  = sys.argv[1]
	sidx = 2
	eidx = len(sys.argv)
	colmerge(outfile, sidx, eidx)

if __name__=="__main__":
	main()

