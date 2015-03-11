import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re
from collections import defaultdict
import numpy as np
from sets import Set

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
  perf = defaultdict(lambda: [])
  query_name = None
  qtime = None

  if os.path.isfile(filename):
    with open(filename, 'r') as f:
      for line in f:
        matchObj = re.match(r'----- (.*) -----', line, re.M | re.I)
        if matchObj:
          if query_name:
            perf[query_name] = [qtime] if qtime else ["timeout"]
          query_name = matchObj.group(1)
          qtime = None

        matchObj = re.match(r'Time: (.*) ms', line, re.M | re.I)
        if matchObj:
          qtime = float(matchObj.group(1)) / 1000.0

      if query_name:
        perf[query_name] = [qtime] if qtime else ["timeout"]

  return perf

def avg_runs(org_vals):
  vals = [float(v) for v in org_vals if v != "timeout"]

  if vals != []:
    result = -1.0
    if len(vals) >= 5:
      result = np.average(np.sort(vals)[1:-1])
    elif len(vals) > 0:
      result = np.average(vals)
    return "Avg:", result, "Min:", np.min(vals), "Max:", np.max(vals), "Num:", len(vals), "Timeouts: ", (len(org_vals) - len(vals))
  else:
    return "Timeouts: ", (len(org_vals) - len(vals))

def merge_results(x, y):
  all_keys = Set(x.keys()) | Set(y.keys())
  result = defaultdict(lambda: [])
  for k in all_keys:
    result[k] = x[k] + y[k]
  return result

def main():
  options = parseInput();
  datasets = ["g_plus", "higgs", "socLivejournal", "orkut", "cid-patents", "twitter2010"]
  threads = ["1"]
  num_runs = [str(x) for x in range(1, 8)]

  result = defaultdict(lambda: defaultdict(lambda: []))
  for dataset in datasets:
    for num_threads in threads:
      for num_run in num_runs:
        p = parse_file(options.folder +"/postgres." + dataset + "." + num_threads + "." + num_run + ".log")
        result[dataset, num_threads] = merge_results(result[dataset, num_threads], p)

  for ds, vs in result.iteritems():
    print ds
    print [(q, avg_runs(ts)) for q, ts in vs.iteritems()]

if __name__ == "__main__":
    main()
