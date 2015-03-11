import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re

#special parsers
from emptyheaded.parse_output import getInternalPerformanceInfo 
from average_runs import average_runs
from graphlab.parse_output import getGraphLabPerformance
from galois.parse_output import getGaloisPerformance
from ligra.parse_output import getLigraPerformance
from postgres.parse_output import getPostgresPerformance

def parseInput():
  parser = OptionParser()
  parser.add_option("-f", "--data_folder", dest="folder",
    help="[REQUIRED] input edge list to parse")

  (opts, args) = parser.parse_args()

  missing_options = []
  for option in parser.option_list:
    if re.match(r'^\[REQUIRED\]', option.help) and eval('opts.' + option.dest) == None:
      missing_options.extend(option._long_opts)
    if len(missing_options) > 0:
      parser.error('Missing REQUIRED parameters: ' + str(missing_options))
    
  return opts

def main():
  options = parseInput();

  bfs_details = True

  datasets = ["baidu","california","higgs","flickr","socLivejournal","orkut","cid-patents","pokec","twitter2010","wikipedia"]
  threads = ["1","24","48"]
  layouts = ["uint", "pshort", "hybrid_perf"]
  orderings = ["u_random"] #, "u_the_game", "u_bfs", "u_degree", "u_rev_degree", "u_strong_run"]

  print options.folder
  matchObj = re.match(r'/dfs/scratch0/\w*/output/(\w*)_.*', options.folder, re.M|re.I)
  system = matchObj.group(1)
  print system

  for dataset in datasets:
    for thread in threads:
        for layout in layouts:
            for ordering in orderings:
              fname = os.path.join(options.folder, dataset + "." + thread + "." + layout + "." + ordering + ".log")
              if os.path.isfile(fname):
                f = open(fname)
                perf_info = getInternalPerformanceInfo(f);
                for p in perf_info['perfs']:
                    query = p['query']
                    avg = average_runs(p['perf'])

                    print "query: " + p['query'] + " data: " + dataset + " t: " + thread + " time: " + str(avg)

                print "layout " + layout
                print "ordering " + ordering
                avg_l2 = average_runs(perf_info['l2_misses'])
                avg_l3 = average_runs(perf_info['l3_misses'])
                print "l2 misses: " + str(avg_l2)
                print "l3 misses: " + str(avg_l3)

if __name__ == "__main__":
    main()
