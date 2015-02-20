import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re

#special parsers
from emptyheaded.parse_output import getIInternalPerformanceInfo 
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

  datasets = ["orkut","cid-patents","socLivejournal","higgs"] #,"twitter2010","wikipedia","orkut"]
  orderings = ["u_the_game"]
  layouts = ["uint","hybrid"]
  threads = ["1"]

  my_output = {}
  for dataset in datasets:
    my_output[dataset] = {}
    for thread in threads:
      for ordering in orderings:
        for layout in layouts:
          f = open(options.folder +"/non_simd." + dataset + "." + thread + "." + ordering + "." + layout + ".log", 'r')
          perf_info = getIInternalPerformanceInfo(f);
          non_simd_avg = average_runs(perf_info['perf'])

          f = open(options.folder +"/simd." + dataset + "." + thread + "." + ordering + "." + layout + ".log", 'r')
          perf_info = getIInternalPerformanceInfo(f);
          simd_avg = average_runs(perf_info['perf'])

          f = open(options.folder +"/up_non_simd." + dataset + "." + thread + "." + ordering + "." + layout + ".log", 'r')
          perf_info = getIInternalPerformanceInfo(f);
          up_non_simd_avg = average_runs(perf_info['perf'])

          f = open(options.folder +"/up_simd." + dataset + "." + thread + "." + ordering + "." + layout + ".log", 'r')
          perf_info = getIInternalPerformanceInfo(f);
          up_simd_avg = average_runs(perf_info['perf'])

          my_output[dataset].update({layout:{thread:{'non_simd':str(non_simd_avg),'simd':str(simd_avg),'up_non_simd':str(up_non_simd_avg),'up_simd':str(up_simd_avg)}}})

        #end for layout
      #end for ordering  
    #end for dataset

  for dataset in my_output.keys():
      data =  my_output[dataset]
      #print data
      print dataset + "," + data['hybrid']['1']['simd'] + "," + data['uint']['1']['simd'] + "," + data['hybrid']['1']['non_simd'] + "," + data['uint']['1']['non_simd'] 
      print dataset + "," + data['hybrid']['1']['up_simd'] + "," + data['uint']['1']['up_simd'] + "," + data['hybrid']['1']['up_non_simd'] + "," + data['uint']['1']['up_non_simd'] 


if __name__ == "__main__":
    main()
