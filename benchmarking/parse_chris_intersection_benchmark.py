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
  mem = []
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

  return times, encoding_times, mem

def main():
  options = parseInput();
  f_perf = open('intersection_density_perf.csv', 'w')
  v_perf = open('vintersection_density_perf.csv', 'w')
  d_perf = open('dintersection_density_perf.csv', 'w')

  results = dict()
  mem_results = dict()
  constr_results = dict()
  for filename in os.listdir(options.folder):
    matchObj = re.match(r'(.*)\.(.*)\.(.*)\.(.*)\.(.*)\.log', filename, re.M | re.I)
    if matchObj:
      skew = float(matchObj.group(1))
      lenA = int(matchObj.group(2))
      lenB = int(matchObj.group(3))
      ranges = int(matchObj.group(4))

      if lenA < lenB:
        abs_filename = os.path.join(options.folder, filename)
        times, encoding_times, mem = parse_file(open(abs_filename))

        results[lenA, lenB, skew] = [[t] for t in times]
        #constr_results[densityA, densityB] = [[t] for t in encoding_times]
        #mem_results[density, comp] = mem

if __name__ == "__main__":
    main()
