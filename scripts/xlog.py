#!/usr/bin/python
import sys

def xlog(infile, colnum, title, outfile):
	# open file 
	inf  = open(infile, 'r')
	outf = open(outfile, 'w')

	# print title
	outf.write(title + "\n")

	# extract column from the input file 
	cn = int(colnum) - 1
	inl = inf.readlines()
	for li in range(len(inl)):
		# skip comment line 
		if inl[li][0] == '#':
			continue
		# split a line
		sinl = inl[li].split()
		# are there cn-th element?
		if len(sinl) < cn:
			continue
		# ok, print out the element
		outf.write(sinl[cn] + "\n")
		
	# close file
	outf.close()
	inf.close()

def print_usage():
	print 'DANBI extract n-th column from DANBI log. Usage:\n'
	print '   xlog.py [input-file] [colummn number] [title] [output-file]\n'
		
def main():
	# check number of argument 
	if len(sys.argv) != 5:
		print_usage()
		return

	# get argument
	infile  = sys.argv[1]
	colnum  = sys.argv[2]
	title   = sys.argv[3]
	outfile = sys.argv[4]
	xlog(infile, colnum, title, outfile)

if __name__=="__main__":
	main()

