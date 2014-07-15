import sys
read_file = sys.argv[1]

f = open(read_file,'r')

data = {}
for line in f:
	line = line.replace('\n','')
	list = line.split('\t')
	srcNode = list.pop(0)
	if(srcNode not in data):
		data[srcNode] = set()
	data[srcNode].update(set(list))
	iterList = iter(list)
	for item in iterList:
		if(item not in data):
			data[item] = set()
		data[item].add(srcNode)
f.close()
w = open('adjacencyLists/' + read_file,'w')
for item in data:
	w.write(item + '\t')
	iterInner = iter(data[item])
	for inner in iterInner:
		w.write(inner + '\t')
	w.write('\n')
w.close()
