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
  perf = defaultdict(lambda: [])

  if os.path.isfile(filename):
    with open(filename, 'r') as f:
      for line in f:
        matchObj = re.match(r'(.*) time: (.*)', line, re.M | re.I)
        if matchObj:
          perf[matchObj.group(1)].append(matchObj.group(2))

  return perf

def avg_runs(vals):
  for v in vals:
    if v == "timeout":
      return "timeout"

  vals = [float(v) for v in vals]
  result = -1.0
  if len(vals) >= 5:
    result = np.average(np.sort(vals)[1:-1])
  elif len(vals) > 0:
    result = np.average(vals)
  return "Avg:", result, "Min:", np.min(vals), "Max:", np.max(vals)

def main():
  options = parseInput();
  datasets = ["g_plus", "higgs", "socLivejournal", "orkut", "cid-patents"]
  threads = ["1", "48"]

  result = defaultdict(lambda: {})
  for dataset in datasets:
    for num_threads in threads:
      p = parse_file(options.folder +"/socialite." + dataset + "." + num_threads + ".1.log")
      result[dataset][num_threads] = p

  for ds, vs in result.iteritems():
    print ds
    for k, v in vs.iteritems():
      print k, [(q, avg_runs(ts)) for q, ts in v.iteritems()]

if __name__ == "__main__":
    main()
