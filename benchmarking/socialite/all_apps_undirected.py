import time
import sys
import os

num_runs = int(sys.argv[2])

filename = os.path.join(sys.argv[1], "glab_undirected", "data.txt")

print "Loading data (undirected)"
`edge(int a:0..20000000, (int b)) indexby a, sortby b.
 edgeRaw(int a:0..20000000, (int b)) indexby a, sortby b.
 edgeRaw(a, b) :- l = $read($filename), (v1,v2) = $split(l, " "), a = $toInt(v1), b = $toInt(v2).`

print "Preprocessing data"
`edge(a, b) :- edgeRaw(a, b).
 edge(b, a) :- edgeRaw(a, b).`

`total(int x:0..0, int s).`

print "Triangle counting"
for k in range(0, num_runs):
    print "Run " + str(k)
    `clear total.`

    start = time.time()
    `total(0, $sum(1)) :- edge(x, y), edge(y, z), edge(x, z).`
    print "Triangle time: " + str(time.time() - start)

for i, s in `total(i, s)`:
    print "Triangles: " + str(s)

print "4-clique counting"
for k in range(0, num_runs):
    print "Run " + str(k)
    `clear total.`

    start = time.time()
    `total(0, $sum(1)) :- edge(x, y), edge(y, z), edge(z, w), edge(x, w), edge(x, z), edge(y, w).`
    print "4-clique time: " + str(time.time() - start)

for i, s in `total(i, s)`:
    print "4-cliques: " + str(s)

print "4-cycle counting"
for k in range(0, num_runs):
    print "Run " + str(k)
    `clear total.`

    start = time.time()
    `total(0, $sum(1)) :- edge(x, y), edge(y, z), edge(z, w), edge(x, w), x != y, x != z, x != w, y != z, y != w, z != w.`
    print "4-cycles time: " + str(time.time() - start)

    `cycle(int x, int y, int z, int w).
     cycle(x, y, z, w) :- edge(x, y), edge(y, z), edge(z, w), edge(x, w), x != y, x != z, x != w, y != z, y != w, z != w.`
    for x, y, z, w in `cycle(x, y, z, w)`:
        print x, y, z, w


for i, s in `total(i, s)`:
    print "4-cycles: " + str(s)
