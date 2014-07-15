#f = open('simple.txt','r')
f = open('higgs-social_network.edgelist','r')
w = open('Higgs/higgs-social_network.edgelist_0','w')
w2 = open('higgs-social_network_edgelist-undirected.txt','w')
i = 0
j = 0
s = set([])
for line in f:
	nodes = line.split()
	if(i > 10**8):
		w.close()
		j += 1
		w = open('Higgs/higgs-social_network.edgelist_' + str(j),'w')
		i = 0
	i += 2*len(line)
	if( (int(nodes[0]),int(nodes[1])) not in s):
		w.write(nodes[0] + " " + nodes[1] + '\n')
		w2.write(nodes[0] + " " + nodes[1] + '\n')
		s.add((int(nodes[0]),int(nodes[1])))
	if( (int(nodes[1]),int(nodes[0])) not in s):
		w.write(nodes[1] + " " + nodes[0] + '\n')
		w2.write(nodes[1] + " " + nodes[0] + '\n')
		s.add((int(nodes[1]),int(nodes[0])))
		
w.close()
w2.close()
f.close()
