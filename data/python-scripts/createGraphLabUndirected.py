#f = open('simple.txt','r')
import sys
read_file = sys.argv[1]

f = open(read_file,'r')
w2 = open('graphlab/' + read_file,'w')
i = 0
j = 0
s = set([])
for line in f:
	nodes = line.split()
	if( (nodes[0] != '%') and  ((int(nodes[0]),int(nodes[1])) not in s) and ((int(nodes[1]),int(nodes[0])) not in s)):
		w2.write(nodes[0] + " " + nodes[1] + '\n')
		s.add((int(nodes[0]),int(nodes[1])))
w2.close()
f.close()
