import time
import sys
import os

@returns(int)
def getYear(d):
    return int(d.split('-')[0])

start = time.time()

path = sys.argv[1]
attributes_file = os.path.join(path, "attributes.txt")
edgelist_file = os.path.join(path, "edgelist.txt")

print "Loading data"
`edge(long a, (long b, int year)) indexby a, sortby b.
 edgeRaw(long a, (long b, int year)) indexby a, sortby b.
 node(long n, int place_id) indexby n.
 node(n, place_id) :- l = $read($attributes_file), (v1, v2) = $split(l, "|"), n = $toLong(v1), place_id = $toInt(v2).
 edgeRaw(a, b, year) :- l = $read($edgelist_file), (v1, v2, v3) = $split(l, "|"), a = $toLong(v1), b = $toLong(v2), year = $getYear(v3).`

print "Preprocessing data"
`edge(a, b, year) :- edgeRaw(a, b, year), a < b.
 edge(b, a, year) :- edgeRaw(a, b, year), b < a.`

print "Triangle counting"
start = time.time()
for k in range(0, 10):
    print "Run " + str(k)
    `total(int x:0..0, int s).
     filtered_edge(long a, (long b)) indexby a, sortby b.
     filtered_edge(n1, n2) :- node(n1, p1), node(n2, p2), edge(n1, n2, year), p1 > 100, p2 > 100, year = 2012.
     total(0, $sum(1)) :- filtered_edge(x, y), filtered_edge(y, z), filtered_edge(x, z).`

print "Triangle time: " + str((time.time() - start) / 10)
for i, s in `total(i, s)`:
    print "Triangles: " + str(s)
