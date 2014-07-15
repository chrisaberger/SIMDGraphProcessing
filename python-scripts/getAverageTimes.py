import fileinput
import re
import operator
import sys

w = open(sys.argv[1] + ".tsv",'w')

for i in xrange(0,18,2):
	l = []
	f = open(sys.argv[1] + str(i) + ".txt",'r')
	for line in f:
		matchObj = re.match(r'\[METRICS\]: Latest time for component app: (\d+.\d+)s',line)
		if matchObj:
			l.append(float(matchObj.group(1)))
	if(len(l)!=0):
		 w.write(str(sum(l)/len(l)) + '\t')
	else:
		w.write("0\t")
	f.close()
w.close()
