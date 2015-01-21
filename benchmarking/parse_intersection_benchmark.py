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
  f_perf = open('intersection_perf.csv', 'w')
  f_mem = open('intersection_mem.csv', 'w')
  f_constr = open('intersection_constr.csv', 'w')

  results = dict()
  mem_results = dict()
  constr_results = dict()
  for filename in os.listdir(options.folder):
    matchObj = re.match(r'(.*)_(.*)_(.*).log', filename, re.M | re.I)
    if matchObj:
      density = float(matchObj.group(1))
      comp = float(matchObj.group(2))

      abs_filename = os.path.join(options.folder, filename)
      times, encoding_times, mem = parse_file(open(abs_filename))

      if (density, comp) in results:
        [ts.append(t) for ts, t in zip(results[density, comp], times)]
        [ts.append(t) for ts, t in zip(constr_results[density, comp], encoding_times)]
      else:
        results[density, comp] = [[t] for t in times]
        constr_results[density, comp] = [[t] for t in encoding_times]
        mem_results[density, comp] = mem

  densities_set = set()
  comps_set = set()
  for k, times in results.iteritems():
    results[k] = [average_runs(t) for t in times]
    constr_results[k] = [average_runs(t) for t in times]
    densities_set.add(k[0])
    comps_set.add(k[1])

  densities = sorted(list(densities_set))
  comps = sorted(list(comps_set))
  f_perf.write("\t" + "\t".join([str(x) for x in comps]) + "\n")
  for density in densities:
    line = [density]
    for comp in comps:
      result = results[density, comp]
      if result == []:
        line.append("-1")
      else:
        line.append(np.argmin(result))
    f_perf.write("\t".join([str(x) for x in line]) + "\n")

  f_mem.write("\t" + "\t".join([str(x) for x in comps]) + "\n")
  for density in densities:
    line = [density]
    for comp in comps:
      result = mem_results[density, comp]
      if result == []:
        line.append("-1")
      else:
        line.append(np.argmin(result))
    f_mem.write("\t".join([str(x) for x in line]) + "\n")

  f_constr.write("\t" + "\t".join([str(x) for x in comps]) + "\n")
  for density in densities:
    line = [density]
    for comp in comps:
      result = constr_results[density, comp]
      if result == []:
        line.append("-1")
      else:
        line.append(np.argmin(result))
    f_constr.write("\t".join([str(x) for x in line]) + "\n")

  f_perf.close()
  f_mem.close()
  f_constr.close()


if __name__ == "__main__":
    main()
