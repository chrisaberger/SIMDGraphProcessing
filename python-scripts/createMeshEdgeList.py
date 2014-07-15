import fileinput
import re
import operator
import sys

w = open("mesh_" + sys.argv[1] + ".txt",'w')

for i in xrange(0,int(sys.argv[1]),1):
	for j in xrange(i,int(sys.argv[1]),1):
		if(i != j):
			w.write(str(i) + " " + str(j) + '\n')
w.close()
