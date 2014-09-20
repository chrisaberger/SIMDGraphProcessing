import fileinput
import re
import operator
import sys
import os

#a stupid script to change the number of dynamic chunks in the deg for last multiloop
if len(sys.argv) != 3:
	print "ERROR: Need 2 arguments: DEG and number of dynamic chunks."
	sys.exit(0)

w = open("tmp",'w')
r = open(sys.argv[1],'r')
i = 0

for line in r:
	matchObj = re.search('\"numDynamicChunksValue\":',line)
	if matchObj:
		i += 1
r.close()
r = open(sys.argv[1],'r')
j = 1
for line in r:
    matchObj = re.search('\"numDynamicChunksValue\":',line)
    if(matchObj and j==i):
        j += 1
        w.write('  \"numDynamicChunksValue\": \"' + sys.argv[2] + '\"\n')	
    elif(matchObj):
		j += 1
		w.write(line)
    else:
	    w.write(line)
w.close()
r.close()
os.system('mv tmp ' + sys.argv[1])
