import time
import sys
import os

num_runs = 10

@returns(int)
def getYear(d):
    return int(d.split('-')[0])

start = time.time()

path = sys.argv[1]
attributes_file = os.path.join(path, "attributes.txt")
edgelist_file = os.path.join(path, "edgelist", "data.txt")

print "Loading data"
`edge(long a, (long b, int year)) indexby a, sortby b.
 node(long n, int place_id) indexby n.
 node(n, place_id) :- l = $read($attributes_file), (v1, v2) = $split(l, " "), n = $toLong(v1), place_id = $toInt(v2).
 edge(a, b, year) :- l = $read($edgelist_file), (v1, v2, v3) = $split(l, " "), a = $toLong(v1), b = $toLong(v2), year = $getYear(v3).`

`path(long a, int d).
 total(int x:0..0, int s).
 filtered_edge(long a, (long b)) indexby a, sortby b.
 filtered_node(long n) indexby n.`

for k in range(0, num_runs):
    print "Run " + str(k)
    `clear path.
     clear total.
     clear filtered_edge.
     clear filtered_node.`

    `filtered_node(n) :- node(n, p), p > 500.
     filtered_edge(frm, to) :- edge(frm, to, year), year == 2012, filtered_node(frm), filtered_node(to).`

    `path(b, $min(d)) :- filtered_edge(14293652286639L, b), d = 1;
                      :- path(a, d1), d1 < 4, filtered_edge(a, b), d = d1 + 1.
     total(0, $sum(1)) :- path(x, 4).`

print "4-path time: " + str((time.time() - start) / num_runs)
for i, s in `total(i, s)`:
    print "4-path: " + str(s)
