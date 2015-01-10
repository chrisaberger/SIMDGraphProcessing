import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re
import numpy as np

from average_runs import average_runs

layouts = ["uint", "pshort", "bitset"]

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

def parse_file(f):
  times = []
  for line in f:
    matchObj = re.match(r'Time\[intersect.*: (.*) s', line, re.M|re.I)
    if matchObj:
      times.append(matchObj.group(1))
  return times

def main():
  options = parseInput();

  results = dict()
  for filename in os.listdir(options.folder):
    matchObj = re.match(r'(.*)_(.*)_(.*).log', filename, re.M | re.I)
    if matchObj:
      run_len = int(matchObj.group(1))
      gap_len = int(matchObj.group(2))

      abs_filename = os.path.join(options.folder, filename)
      times = parse_file(open(abs_filename))

      if (run_len, gap_len) in results:
        [ts.append(t) for ts, t in zip(results[run_len, gap_len], times)]
      else:
        results[run_len, gap_len] = [[t] for t in times]

  run_lens_set = set()
  gap_lens_set = set()
  for k, times in results.iteritems():
    results[k] = [average_runs(t) for t in times]
    run_lens_set.add(k[0])
    gap_lens_set.add(k[1])

  run_lens = sorted(list(run_lens_set))
  gap_lens = sorted(list(gap_lens_set))
  for run_len in run_lens:
    line = []
    for gap_len in gap_lens:
      line.append(np.argmin(results[run_len, gap_len]))
    print " ".join([str(x) for x in line])

if __name__ == "__main__":
    main()
