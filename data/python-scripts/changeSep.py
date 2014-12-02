import sys

read_file = sys.argv[1]
out_file = sys.argv[2]

sep = None
if len(sys.argv) > 3:
    sep = sys.argv[3]

f = open(read_file, 'r')
w = open(out_file, 'w')

for line in f:
    line = line.strip()
    nodes = line.split() if not sep else line.split(sep)
    w.write(" ".join(nodes) + '\n')
w.close()
f.close()
