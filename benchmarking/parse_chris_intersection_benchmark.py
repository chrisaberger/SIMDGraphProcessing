import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re
import numpy as np

from average_runs import average_runs

algos_to_layout = [0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]

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

def den_to_card(d):
  return int(d * 1000000)

def main():
  options = parseInput();
  f_perf = open('intersection_density_perf.csv', 'w')
  v_perf = open('vintersection_density_perf.csv', 'w')
  d_perf = open('dintersection_density_perf.csv', 'w')
  diff_worst_best = open('diff_worst_best.csv', 'w')

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
  current_line = []
  num_intersections = 0
  for k in my_keys:
    if(k[0] == prev or prev == -1):
      num_intersections += 1
      current_line.append(str(den_to_card(k[1])))
      prev = k[0]

  header = "\t" + "\t".join(current_line) + "\n"
  f_perf.write(header)
  d_perf.write(header)
  diff_worst_best.write(header)

  numShift = 0
  lastDen = -1
  first = True
  diff = 1e6

  key_str = str(den_to_card(my_keys[0][0]))
  current_line = [key_str]
  current_line2 = [key_str]
  diff_worst_best_line = [key_str]

  for k in my_keys:
    times = results[k]
    denA = k[0]
    denB = k[1]

    if(denA != lastDen and lastDen != -1):
      f_perf.write("\t".join(current_line) + "\n")
      d_perf.write("\t".join(current_line2) + "\n")
      diff_worst_best.write("\t".join(diff_worst_best_line) + "\n")

      key_str = str(den_to_card(denA))
      current_line = [key_str]
      current_line2 = [key_str]
      diff_worst_best_line = [key_str]

      numShift += 1
      for i in range(0,numShift):
        current_line.append('-1')
        current_line2.append('-1')
        diff_worst_best_line.append('-1')

    lastDen = denA

    curTup = k
    results[curTup] = [average_runs(t) for t in times]
    v_perf.write(str(curTup[0]) + "|" + str(curTup[1]) + ',')

    results_wo_hybrid = results[curTup][0:-1]
    v_perf.write(",".join([str(x) for x in results[curTup]]) + '\n')
    current_line.append(str(algos_to_layout[np.argmin(results_wo_hybrid)]))
    current_line2.append(str(results[curTup][-1] / np.min(results_wo_hybrid)))
    diff_worst_best_line.append(str(max(results_wo_hybrid) / np.min(results_wo_hybrid)))

  f_perf.write("\t".join(current_line) + "\n")
  d_perf.write("\t".join(current_line2) + "\n")
  diff_worst_best.write("\t".join(diff_worst_best_line) + "\n")
  diff_worst_best.close()

if __name__ == "__main__":
    main()
