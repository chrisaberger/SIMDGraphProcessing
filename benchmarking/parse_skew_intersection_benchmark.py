import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re
import numpy as np
from collections import defaultdict

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
  labels = []
  times = []
  encoding_times = []
  mem = []

  if os.path.isfile(filename):
    with open(filename, 'r') as f:
      for line in f:
        matchObj = re.match(r'label: (.*)', line, re.M|re.I)
        if matchObj:
          labels.append(matchObj.group(1))

        matchObj = re.match(r'Time\[intersect.*: (.*) s', line, re.M|re.I)
        if matchObj:
          times.append(matchObj.group(1))

        matchObj = re.match(r'Time\[encoding.*: (.*) s', line, re.M|re.I)
        if matchObj:
          encoding_times.append(matchObj.group(1))

        matchObj = re.match(r'Size: (.*)', line, re.M|re.I)
        if matchObj:
          mem.append(int(matchObj.group(1)))
  # else:
  #  print "Warning! " + filename + " not found"

  return labels, times, encoding_times, mem

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

  labels = []
  ranges = ["1000000", "5000000", "10000000"]
  cards = ["8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096", "8192", "16384", "32768", "65536", "131072", "262144"]
  skews = ["0.0", "0.0001", "0.0002", "0.0004", "0.0008", "0.0016", "0.0032", "0.0064", "0.0128", "0.0256", "0.0512", "0.1024", "0.00625", "0.0125", "0.025", "0.05", "0.1", "0.2", "0.4", "0.8"]

  results = defaultdict(lambda: [])
  for run in range(1, 8):
    for set_range in ranges:
      for card in cards:
        for skew in skews:
          l, p, _, _ = parse_file(os.path.join(options.folder, set_range + "_" + card + "_" + skew + "_" + str(run) + ".log"))
          if len(labels) < len(l):
            labels = l
          results[set_range, card, skew].append(p)

  for set_range in ranges:
    print set_range
    for skew in skews:
      print skew
      print "\t" + "\t".join(labels)
      for card in cards:
        max_len = np.max(map(lambda x: len(x), results[set_range, card, skew]))
        if max_len > 0:
          agg_results = [[] for x in range(max_len)]
          for r in results[set_range, card, skew]:
            if len(r) == max_len:
              [agg_results[i].append(x) for i, x in enumerate(r)]
          print card + "\t" + "\t".join([str(avg_runs(vs)) for vs in agg_results])
      print
      print

if __name__ == "__main__":
    main()
