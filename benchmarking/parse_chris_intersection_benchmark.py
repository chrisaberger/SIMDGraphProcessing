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
  diff_perf = open('diff_density_perf.csv', 'w')
  v_perf = open('vintersection_density_perf.csv', 'w')

  skews=["0.00002","0.00004","0.00008","0.00016","0.00032"] #"0.00128","0.00256","0.00512","0.01024","0.02048"]
  lensA=["20","40","80","160","320","1280","2560","5120","10240","20480","40960","163840","327680","655360","1310720"]
  lensB=["20","40","80","160","320","1280","2560","5120","10240","20480","40960","163840","327680","655360","1310720"]
  data_range = "10000000"

  uint_dict = {}
  min_types = {}
  min_times = {}

  for skew in skews:
    f_perf = open('intersection_density_perf_' + skew + '.csv', 'w')
    f_perf.write('\t' + "\t".join(lensB) +  '\n')
    d_perf = open('diff_intersection_density_perf_' + skew + '.csv', 'w')
    d_perf.write('\t' + "\t".join(lensB) +  '\n')
    for lenA in lensA:
      type_output = []
      times_output = []
      for lenB in lensB:
        if int(lenA) <= int(lenB):
          print options.folder + '/' + skew + "." + lenA + "." + lenB + "." +  data_range + ".0.log"
          filename = open(options.folder + '/' + skew + "." + lenA + "." + lenB + "." + data_range + ".0.log", 'r')
          times,encoding_times,mem = parse_file(filename)

          #First deal with the UINT algos
          uint_dict.update({skew+"_"+lenA+"_"+lenB:[times[0],times[1],times[2],times[3],times[4]]})
          min_uint = 100000000.0          
          for i in range(0,5):
            if float(times[i]) < min_uint:
              min_uint = float(times[i])

          min_time = min_uint 
          min_type = 0
          for i in range(5,10):
            print str(min_time) + " " + str(times[i])
            if(float(times[i]) < min_time):
              min_time = float(times[i])
              min_type = i-4

          relative_perf = float(min_uint)/float(min_time)

          type_output.append(str(min_type))
          times_output.append(str(relative_perf))

          print times

          min_types.update({skew+"_"+lenA+"_"+lenB:min_type})
          min_times.update({skew+"_"+lenA+"_"+lenB:relative_perf})
        else:
          type_output.append("-1")
          times_output.append("-1")

      f_perf.write(lenA + '\t' + "\t".join(type_output) +  '\n')
      d_perf.write(lenA + '\t' + "\t".join(times_output) +  '\n')



if __name__ == "__main__":
    main()
