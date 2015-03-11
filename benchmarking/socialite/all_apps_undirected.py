import threading
from java.lang import Thread as JThread, InterruptedException
import time
import sys
import os
import signal

# Timeout for queries
timeout = 7200

# Here come the queries
def triangle_counting():
  `total(0, $sum(1L)) :- edge(x, y), edge(y, z), edge(x, z).`

def clique_counting():
  `total(0, $sum(1L)) :- edge(x, y), edge(y, z), edge(z, w), edge(x, w), edge(x, z), edge(y, w).`

def cycle_counting():
  `total(0, $sum(1L)) :- edge(x, y), edge(y, z), edge(z, w), edge(x, w), x != z.`

def lollipop_counting():
  `total(0, $sum(1L)) :- uedge(x, y), uedge(y, z), uedge(x, z), uedge(x, w).`

def tadpole_counting():
  `total(0, $sum(1L)) :- uedge(x, y), uedge(y, z), x != z, uedge(z, w), y != w, uedge(x, w), w < y, uedge(x, a), a != y, a != z, a != w, uedge(a, b), b != x.`

def barbell_counting():
  `total(0, $sum(1L)) :- uedge(x, y), uedge(y, z), uedge(x, z), uedge(x, a), uedge(a, b), uedge(b, c), uedge(a, c), a != y, a != z, y < z, b < c, x < a.`

def fish_counting():
  `total(0, $sum(1L)) :- uedge(x, y), uedge(y, z), uedge(x, z), y < z, uedge(x, w), w != y, w != z, uedge(w, a), a != x, a != y, a!= z, uedge(a, b), uedge(x, b), b < w, b != z, b != y.`

def benchmark_query(name, fn, num_runs):
  print

  for k in range(0, num_runs):
    print "Starting run " + str(k)
    `clear total.`

    start = time.time()
    t = threading.Thread(target=fn, args=None)
    t.setDaemon(True)
    t.start()
    t.join(timeout)

    if t.isAlive():
      print name + " time: timeout"

      # Yeah, this is not exactly beautiful but there seems to be no easy way
      # to kill the threads spawned by SociaLite... 
      os.kill(os.getpid(), signal.SIGINT)
      sys.exit(-1)
    else:
      print name + " time: " + str(time.time() - start)
      for i, s in `total(i, s)`:
        print name + " count: " + str(s)

if __name__ == '__main__':
  filename = os.path.join(sys.argv[1], "glab_undirected", "data.txt")
  num_runs = int(sys.argv[2])

  print "Loading data (undirected)"
  `edge(int a:0..20000000, (int b)) indexby a, sortby b.
   uedge(int a:0..20000000, (int b)) indexby a, sortby b.
   edgeRaw(int a:0..20000000, (int b)) indexby a, sortby b.
   total(int x:0..0, long s).
   edgeRaw(a, b) :- l = $read($filename), (v1,v2) = $split(l, " "), a = $toInt(v1), b = $toInt(v2).`

  print "Preprocessing data"
  `edge(a, b) :- edgeRaw(a, b), a < b.
   edge(b, a) :- edgeRaw(a, b), b < a.
   uedge(a, b) :- edgeRaw(a, b).
   uedge(b, a) :- edgeRaw(a, b).`

  queries = {
      "triangle_counting": triangle_counting,
      "clique_counting": clique_counting,
      "cycle_counting": cycle_counting,
      "lollipop_counting": lollipop_counting,
      "tadpole_counting": tadpole_counting,
      "barbell_counting": barbell_counting,
      "fish_counting": fish_counting }

  query = sys.argv[3]
  benchmark_query(query, queries[query], num_runs)
