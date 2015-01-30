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
    matchObj = re.match(r'(.*)_(.*)_(.*).log', filename, re.M | re.I)
    if matchObj:
      densityA = float(matchObj.group(1))
      densityB = float(matchObj.group(2))

      abs_filename = os.path.join(options.folder, filename)
      times, encoding_times, mem = parse_file(open(abs_filename))

      if (densityA, densityB) in results:
        [ts.append(t) for ts, t in zip(results[densityA, densityB], times)]
        [ts.append(t) for ts, t in zip(constr_results[densityA, densityB], encoding_times)]
      else:
        results[densityA, densityB] = [[t] for t in times]
        constr_results[densityA, densityB] = [[t] for t in encoding_times]
        #mem_results[density, comp] = mem

  densities_set = set()
  comps_set = set()

  my_keys = results.keys()
  my_keys.sort()

  prev = -1
  f_perf.write('\t')
  current_line = []
  num_intersections = 0
  for k in my_keys:
    if(k[0] == prev or prev == -1):
      num_intersections += 1
      current_line.append(str(k[1]))
      prev = k[0]
  f_perf.write("\t".join(current_line)) 
  f_perf.write('\n')

  d_perf.write("\t".join(current_line)) 
  d_perf.write('\n')

  current_line = []
  current_line2 = []

  numShift = 0
  lastDen = -1
  first = True
  diff = 1e6

  current_line.append(str(my_keys[0][0]))
  current_line2.append(str(my_keys[0][0]))

  for k in my_keys:
    times = results[k]
    denA = k[0]
    denB = k[1]

    if(denA != lastDen and lastDen != -1):
      f_perf.write("\t".join(current_line))
      f_perf.write("\n")

      d_perf.write("\t".join(current_line2))
      d_perf.write("\n")

      current_line = []
      current_line.append(str(denA))
      current_line2 = []
      current_line2.append(str(denA))

      numShift += 1
      for i in range(0,numShift):
        current_line.append('-1')
        current_line2.append('-1')

    lastDen = denA

    curTup = k
    results[curTup] = [average_runs(t) for t in times]
    v_perf.write(str(curTup[0]) + "|" + str(curTup[1]) + ',')

    lowest = 1e6
    lowest_counter = 0
    counter = 0
    for kk in results[curTup]:
      v_perf.write(str(kk) + ',')
      if(kk < lowest and counter < (num_intersections-1)):
        lowest_counter = counter
        lowest = kk
      diff = float((kk)/lowest) 
      counter += 1

    v_perf.write('\n')
    current_line.append(str(lowest_counter))
    current_line2.append(str(diff))

if __name__ == "__main__":
    main()
