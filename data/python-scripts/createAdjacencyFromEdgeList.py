import sys
from sets import Set
read_file = sys.argv[1]

f = open(read_file,'r')

s = Set([])
data = {}
for line in f:
	line = line.rstrip()
	list = line.split(' ')
	s.add(list.pop(0))
	s.add(list.pop(0))
f.close()

print len(s)