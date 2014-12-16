import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re

#special parsers
from emptyheaded.parse_output import getInternalPerformanceInfo 
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

def main():
  options = parseInput();

  runs = {"200000": 
    ["1000000","2000000","3000000","4000000","6000000","8000000","16000000","32000000","64000000","128000000","256000000","512000000","1250000000"]}

  layouts = ["a32","a16","hybrid_perf","hybrid_comp","bp","v"] #"hybrid_comp",
  threads = ["1","24","48"]

  for vertex in runs:
    perf_file = open(vertex+"_perf.csv", 'w')
    comp_file = open(vertex+"_comp.csv", 'w')

    perf_file.write(",,")
    comp_file.write(",,")
    
    for edge in runs[vertex]:
      perf_file.write(edge + ",")
      comp_file.write(edge + ",")
    perf_file.write("\n")
    comp_file.write("\n")

    for thread in threads:
      for layout in layouts:
        perf_file.write(thread + "," + layout + ",")
        comp_file.write(thread + "," + layout + ",")
        for edge in runs[vertex]:
          f = open(options.folder + "/v" + vertex + ".e" + edge + "." + layout + "." + thread + ".log")
          perf_info = getInternalPerformanceInfo(f,True);
          avg = average_runs(perf_info['perf'])
          perf_file.write(str(avg) + ",")
          comp_file.write(str(perf_info['bytes']) + ",")
        perf_file.write("\n")
        comp_file.write("\n")

if __name__ == "__main__":
    main()
