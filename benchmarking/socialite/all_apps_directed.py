import time
import sys

num_runs = 10

start = time.time()

filename = sys.argv[1]

print "Loading data (directed)"
`edge(int a:0..2000000, (int b))  indexby a, sortby b.
 edge(a, b) :- l = $read($filename), (v1,v2) = $split(l, " "), a = $toInt(v1), b = $toInt(v2).`

print "4-path"
start = time.time()
for k in range(0, num_runs):
    print "Run " + str(k)
    `total(int x:0..0, int s).
     total(0, $sum(1)) :- edge(x, y), x == 222074, edge(y, z), edge(z, w), edge(w, v), x != y, x != z, x != w, x != v, y != z, y != w, y != v, z != w, z != v, w != v.`

print "4-path time: " + str((time.time() - start) / num_runs)
for i, s in `total(i, s)`:
    print "4-path: " + str(s / num_runs)
