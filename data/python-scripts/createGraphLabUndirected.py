import sys

read_file = sys.argv[1]
out_file = sys.argv[2]

sep = None
if len(sys.argv) > 3:
    sep = sys.argv[3]

f = open(read_file, 'r')
w2 = open(out_file, 'w')
s = {}

for line in f:
    line = line.strip()
    nodes = line.split() if not sep else line.split(sep)
    source = int(nodes[0])
    dest = int(nodes[1])
    if(source not in s.keys()):
        s[source] = len(s.keys())
    if(dest not in s.keys()):
        s[dest] = len(s.keys())
    #if((source != '%') and ((source, dest) not in s) and ((dest, source) not in s)):
    w2.write(str(s[source]) +' ' + str(s[dest]) + '\n')
w2.close()
f.close()
