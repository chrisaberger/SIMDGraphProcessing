import time
import sys

num_runs = 10

start = time.time()

filename = sys.argv[1]

print "Loading data (directed)"
`edge(int a:0..2000000, (int b)).
 edge(a, b) :- l = $read($filename), (v1,v2) = $split(l, " "), a = $toInt(v1), b = $toInt(v2).`

print "4-path"
start = time.time()
for k in range(0, num_runs):
    print "Run " + str(k)
    `path(int a:0..2000000, int d).
     path(b, $min(d)) :- edge(222074, b), d = 1;
                      :- path(a, d1), edge(a, b), d = d1 + 1.`

print "4-path time: " + str((time.time() - start) / num_runs)
for i, s in `total(i, s)`:
    print "4-path: " + str(s / num_runs)
