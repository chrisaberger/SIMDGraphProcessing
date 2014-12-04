import time
import sys
import os

num_runs = 10

start = time.time()

filename = os.path.join(sys.argv[1], "edgelist", "data.txt")

print "Loading data (directed)"
`edge(int a:0..20000000, (int b)).
 edge(a, b) :- l = $read($filename), (v1,v2) = $split(l, " "), a = $toInt(v1), b = $toInt(v2).`

print "4-path"
start = time.time()

`path(int a:0..20000000, int d).
 total(int x:0..0, int s).`

for k in range(0, num_runs):
    print "Run " + str(k)
    `clear path.
     clear total.
     path(b, $min(d)) :- edge(222074, b), d = 1;
                      :- path(a, d1), d1 < 4, edge(a, b), d = d1 + 1.
     total(0, $sum(1)) :- path(x, 4).`

print "4-path time: " + str((time.time() - start) / num_runs)
for i, s in `total(i, s)`:
    print "4-path: " + str(s)
