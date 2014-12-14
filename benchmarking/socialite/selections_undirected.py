import time
import sys
import os

num_runs = int(sys.argv[2])

@returns(int)
def getYear(d):
    return int(d.split('-')[0])

start = time.time()

path = sys.argv[1]
attributes_file = os.path.join(path, "attributes.txt")
edgelist_file = os.path.join(path, "glab_undirected", "data.txt")

print "Loading data"
`edge(long a, (long b, int year)) indexby a, sortby b.
 edgeRaw(long a, (long b, int year)) indexby a, sortby b.
 node(long n, int place_id) indexby n.
 node(n, place_id) :- l = $read($attributes_file), (v1, v2) = $split(l, " "), n = $toLong(v1), place_id = $toInt(v2).
 edgeRaw(a, b, year) :- l = $read($edgelist_file), (v1, v2, v3) = $split(l, " "), a = $toLong(v1), b = $toLong(v2), year = $getYear(v3).`

print "Preprocessing data"
`edge(a, b, year) :- edgeRaw(a, b, year), a < b.
 edge(b, a, year) :- edgeRaw(a, b, year), b < a.`

`total(int x:0..0, int s).
 filtered_edge(long a, (long b)) indexby a, sortby b.
 filtered_node(long n) indexby n.`

print "Triangle counting"
for k in range(0, num_runs):
    print "Run " + str(k)
    `clear total.
     clear filtered_edge.
     clear filtered_node.`

    start = time.time()
    `filtered_node(n) :- node(n, p), p > 500.
     filtered_edge(frm, to) :- edge(frm, to, year), year == 2012, filtered_node(frm), filtered_node(to).
     total(0, $sum(1)) :- filtered_edge(x, y), filtered_edge(y, z), filtered_edge(x, z).`

    print "Triangle time: " + str(time.time() - start)

for i, s in `total(i, s)`:
    print "Triangles: " + str(s)

print "4-clique counting"
for k in range(0, num_runs):
    print "Run " + str(k)
    `clear total.
     clear filtered_edge.
     clear filtered_node.`

    start = time.time()
    `filtered_node(n) :- node(n, p), p > 500.
     filtered_edge(frm, to) :- edge(frm, to, year), year == 2012, filtered_node(frm), filtered_node(to).
     total(0, $sum(1)) :- filtered_edge(x, y), filtered_edge(y, z), filtered_edge(z, w), filtered_edge(x, w), filtered_edge(x, z), filtered_edge(y, w).`

    print "4-clique time: " + str(time.time() - start)

for i, s in `total(i, s)`:
    print "4-cliques: " + str(s)

print "4-cycle counting"
for k in range(0, num_runs):
    print "Run " + str(k)
    `clear total.
     clear filtered_edge.
     clear filtered_node.`

    start = time.time()
    `filtered_node(n) :- node(n, p), p > 500.
     filtered_edge(frm, to) :- edge(frm, to, year), year == 2012, filtered_node(frm), filtered_node(to).
     total(0, $sum(1)) :- filtered_edge(x, y), filtered_edge(y, z), filtered_edge(z, w), filtered_edge(x, w).`

    print "4-cycles time: " + str(time.time() - start)

for i, s in `total(i, s)`:
    print "4-cycles: " + str(s)
