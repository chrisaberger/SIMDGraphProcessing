import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re
import numpy as np

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

def parse_file(f):
  times = []
  encoding_times = []

  for line in f:
    matchObj = re.match(r'Time\[intersect.*: (.*) s', line, re.M|re.I)
    if matchObj:
      times.append(float(matchObj.group(1)))

    matchObj = re.match(r'Time\[encoding.*: (.*) s', line, re.M|re.I)
    if matchObj:
      encoding_times.append(float(matchObj.group(1)))

  return times, encoding_times

def main():
  intersections = ["UINTEGER_UINTEGER_IBM", "UINTEGER_UINTEGER_STANDARD", "UINTEGER_UINTEGER_V3", "UINTEGER_UINTEGER_V1", "UINTEGER_UINTEGER_GALLOP", "PSHORT_PSHORT", "BITSET_BITSET", "UINTEGER_PSHORT", "UINTEGER_BITSET", "PSHORT_BITSET"]

  options = parseInput()

  lens = set()
  sweep_lens = set()
  results_lens = dict()
  for filename in os.listdir(options.folder):
    matchObj = re.match(r'(.*)_(.*)_(.*).log', filename, re.M | re.I)
    if matchObj:
      lenA = int(matchObj.group(1))
      lenB = int(matchObj.group(2))
      abs_filename = os.path.join(options.folder, filename)
      times, _ = parse_file(open(abs_filename))
      lens.add(lenA)
      sweep_lens.add(lenB)

      if (lenA, lenB) in results_lens:
        [ts.append(t) for ts, t in zip(results_lens[lenA, lenB], times)]
      else:
        results_lens[lenA, lenB] = [[t] for t in times]

  for k in results_lens:
    results_lens[k] = [np.average(t) for t in results_lens[k]]

  for l in sorted(lens):
    with open('results_len_len_' + str(l), 'w') as f:
      f.write("Range" + "\t" + "\t".join(intersections) + "\n")
      for sl in sorted(sweep_lens):
        f.write(str(sl) + "\t" + "\t".join([str(x) for x in results_lens[l, sl]]) + "\n")

if __name__ == "__main__":
    main()
