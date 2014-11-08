import time
import sys

start = time.time()


filename = sys.argv[1]

print "Loading data"
`edge(int a, int b).
 edgeRaw(int a, int b).
 edgeRaw(a, b) :- l = $read($filename), (v1,v2) = $split(l, "\t"), a = $toInt(v1), b = $toInt(v2).
 edge(a, b) :- edgeRaw(a, b), a < b.
 edge(b, a) :- edgeRaw(a, b), b < a.`

print "Data loaded"
start = time.time()

`total(int x, int s).
 total(0, $sum(1)) :- edge(x, y), edge(y, z), edge(z, w), edge(x, w).`
print "Time: " + str(time.time() - start)

for i, s in `total(i, s)`:
    print "Result: " + str(s)
