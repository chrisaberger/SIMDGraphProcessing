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
  times = []
  encoding_times = []
  mem = []

  if os.path.isfile(filename):
    with open(filename, 'r') as f:
      for line in f:
        matchObj = re.match(r'Time\[intersect.*: (.*) s', line, re.M|re.I)
        if matchObj:
          times.append(matchObj.group(1))

        matchObj = re.match(r'Time\[encoding.*: (.*) s', line, re.M|re.I)
        if matchObj:
          encoding_times.append(matchObj.group(1))

        matchObj = re.match(r'Size: (.*)', line, re.M|re.I)
        if matchObj:
          mem.append(int(matchObj.group(1)))
  else:
    print "Warning! " + filename + " not found"

  return times, encoding_times, mem

def main():
  options = parseInput();

  ranges = ["1000000", "5000000", "10000000"]
  cards = ["512", "1024", "2048", "4096", "8192", "16384", "32768", "65536", "131072", "262144"]
  skews = ["0.0", "0.00002", "0.00032", "0.02048"]

  results = defaultdict(lambda: [])
  for set_range in ranges:
    for card in cards:
      for skew in skews:
        p, _, _ = parse_file(os.path.join(options.folder, set_range + "_" + card + "_" + skew + "_0.log"))
        results[card, skew].append(p)

  for set_range in ranges:
    print set_range
    for skew in skews:
      print skew
      for card in cards:
        print card + "\t" + "\t".join(results[card, skew][0])
      print
      print

if __name__ == "__main__":
    main()
