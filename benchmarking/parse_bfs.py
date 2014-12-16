from __future__ import print_function
import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re

# special parsers
from emptyheaded.parse_output import getInternalPerformanceInfo 
from average_runs import average_runs

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

  datasets = ["orkut", "socLivejournal", "cid-patents", "twitter2010" ]
  layouts = ["uint", "pshort", "hybrid_perf", "hybrid_comp", "bp", "v" ]
  threads = ["1","24","48"]

  for thread in threads:
    bfs_file = open("bfs." + thread + ".csv", 'w')
    print("\t" + "\t".join(layouts), file=bfs_file)
    for dataset in datasets:
      row = [dataset]
      for layout in layouts:
        fname = os.path.join(options.folder, dataset + "." + thread + "." + layout + "..log")
        if os.path.isfile(fname):
          f = open(fname)
          perf_info = getInternalPerformanceInfo(f);

          for p in perf_info['perfs']:
              query = p['query']
              if query == "bfs":
                avg = average_runs(p['perf'])
                row.append(str(avg))
        else:
          row.append("-1")

      print("\t".join(row), file=bfs_file)
    print("", file=bfs_file)
             # print "query: " + p['query'] + " data: " + dataset + " t: " + thread + " layout: " + layout + " time: " + str(avg)

if __name__ == "__main__":
    main()
