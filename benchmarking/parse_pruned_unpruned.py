import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re
from collections import defaultdict
import numpy as np

#special parsers
from emptyheaded.parse_output import getIInternalPerformanceInfo 
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

def parse_file(filename):
  perf = []
  if os.path.isfile(filename):
    with open(filename, 'r') as f:
      for line in f:
        matchObj = re.match(r'Time\[UNDIRECTED TRIANGLE.*: (.*) s', line, re.M | re.I)
        if matchObj:
          perf.append(matchObj.group(1))

  return perf

def avg_runs(vals):
  vals = [float(v) for v in vals]
  result = -1.0
  if len(vals) >= 5:
    result = np.average(np.sort(vals)[1:-1])
  elif len(vals) > 0:
    result = np.average(vals)
  return result

def main():
  options = parseInput();

  datasets = ["g_plus", "higgs", "socLivejournal", "orkut", "cid-patents", "twitter2010","wikipedia"]
  orderings = ["random", "degree"]
  data_types = ["pruned", "unpruned"]
  layouts = ["uint","hybrid"]
  threads = ["1"]
  runs = [str(x) for x in range(1, 8)]

  result = defaultdict(lambda: defaultdict(lambda: []))
  for dataset in datasets:
    for run in runs:
      for ordering in orderings:
        for datatype in data_types:
          for layout in layouts:
            
            p = parse_file(options.folder +"/" + dataset + "_" + run + "_" + datatype + "_" + layout + "_" + ordering + ".log")
            result[dataset][datatype, layout, ordering] += p

  for ds, vs in result.iteritems():
    print ds
    for k, v in vs.iteritems():
      print k, avg_runs(v)

if __name__ == "__main__":
    main()
