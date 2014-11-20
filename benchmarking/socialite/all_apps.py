import time
import sys

num_runs = 10

start = time.time()

filename = sys.argv[1]

print "Loading data"
`edge(int a:0..2000000, (int b))  indexby a, sortby b.
 edgeRaw(int a:0..2000000, (int b))  indexby a, sortby b.
 edgeRaw(a, b) :- l = $read($filename), (v1,v2) = $split(l, " "), a = $toInt(v1), b = $toInt(v2).`

print "Preprocessing data"
`edge(a, b) :- edgeRaw(a, b), a < b.
 edge(b, a) :- edgeRaw(a, b), b < a.`

print "Triangle counting"
start = time.time()
for k in range(0, num_runs):
    print "Run " + str(k)
    `total(int x:0..0, int s).
     total(0, $sum(1)) :- edge(x, y), edge(y, z), edge(x, z).`

print "Triangle time: " + str((time.time() - start) / num_runs)
for i, s in `total(i, s)`:
    print "Triangles: " + str(s / num_runs)

print "4-clique counting"
start = time.time()
for k in range(0, num_runs):
    print "Run " + str(k)
    `total(int x:0..0, int s).
     total(0, $sum(1)) :- edge(x, y), edge(y, z), edge(z, w), edge(x, w), edge(x, z), edge(y, w).`

print "4-clique time: " + str((time.time() - start) / num_runs)
for i, s in `total(i, s)`:
    print "4-cliques: " + str(s / num_runs)

print "4-cycle counting"
start = time.time()
for k in range(0, num_runs):
    print "Run " + str(k)
    `total(int x:0..0, int s).
     total(0, $sum(1)) :- edge(x, y), edge(y, z), edge(z, w), edge(x, w).`

print "4-cycles time: " + str((time.time() - start) / num_runs)
for i, s in `total(i, s)`:
    print "4-cycles: " + str(s / num_runs)

print "4-path"
start = time.time()
for k in range(0, num_runs):
    print "Run " + str(k)
    `total(int x:0..0, int s).
     total(0, $sum(1)) :- edge(x, y), edge(y, z), edge(z, w), edge(x, w).`

print "4-path: " + str((time.time() - start) / num_runs)
for i, s in `total(i, s)`:
    print "4-path: " + str(s / num_runs)
