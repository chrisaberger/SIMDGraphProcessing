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

  parser.add_option("-p", "--perf_output", dest="perf_output",
    help="[REQUIRED] number of thread to run with")

  parser.add_option("-c", "--comp_output", dest="comp_output",
    help="[REQUIRED] number of thread to run with")

  parser.add_option("-s", "--simd_output", dest="simd_output",
    help="[REQUIRED] number of thread to run with")

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

  datasets = ["higgs"]
  orderings = ["u_the_game","u_degree","u_bfs","u_random","u_rev_degree","u_strong_run"]
  layouts = ["uint","pshort","hybrid_perf","hybrid_comp","bp","v"]
  threads = ["1","24","48"]

  perf_file = open(options.perf_output, 'w')
  comp_file = open(options.comp_output, 'w')
  simd_file = open(options.simd_output, 'w')

  perf_file.write(",,")
  for dataset in datasets:
    for layout in layouts: 
      perf_file.write(dataset + ",")
  perf_file.write("\n")

  simd_file.write(",,,simd,non-simd\n")
  perf_file.write(",,")
  for dataset in datasets:
    for layout in layouts: 
      perf_file.write(layout + ",")
  perf_file.write("\n")

  comp_file.write(",")
  for ordering in orderings:
    for layout in layouts:
      comp_file.write(ordering + ",")
  comp_file.write("\n")

  comp_file.write(",")
  for ordering in orderings:
    for layout in layouts:
      comp_file.write(layout + ",")
  comp_file.write("\n")

  num_bytes = -1
  num_edges = -1
  print_to_comp_file = True;
  for dataset in datasets:
    for thread in threads:
      for ordering in orderings:
        for layout in layouts:
          f = open(options.folder +"/" + dataset + "." + thread + "." + layout + "." + ordering +".log", 'r')
          perf_info = getInternalPerformanceInfo(f,print_to_comp_file);
          avg = average_runs(perf_info['perf'])
          if layout == "hybrid_perf":
            f = open(options.folder +"/nonsimd_" + dataset + "." + thread + "." + layout + "." + ordering +".log", 'r')
            hperf_info = getInternalPerformanceInfo(f);
            havg = average_runs(hperf_info['perf'])
            simd_file.write(dataset + "," + thread + "," + ordering + "," + str(avg) + "," + str(havg) + "\n")
        #end for layout
      #end for ordering  
    #end for dataset

  print_to_comp_file = True;
  for thread in threads:
    if print_to_comp_file:
      for dataset in datasets:
        comp_file.write(dataset+ ",")
        for ordering in orderings:
            for layout in layouts:          
              f = open(options.folder +"/" + dataset + "." + thread + "." + layout + "." + ordering +".log", 'r')
              perf_info = getInternalPerformanceInfo(f,print_to_comp_file);
              bits_per_edge = (float(perf_info['bytes'])*8.0)/float(perf_info['edges'])
              comp_file.write(str(bits_per_edge) + ",")
            #end for layout
        #end for ordering 
        comp_file.write("\n")
      #end for dataset
    print_to_comp_file = False

  num_bytes = -1
  num_edges = -1
  print_to_comp_file = True;
  for thread in threads:
    for ordering in orderings:
      perf_file.write("t" +thread + "," + ordering + ",")
      for dataset in datasets:
        for layout in layouts:          
          f = open(options.folder +"/" + dataset + "." + thread + "." + layout + "." + ordering +".log", 'r')
          perf_info = getInternalPerformanceInfo(f,print_to_comp_file);
          avg = average_runs(perf_info['perf'])
          perf_file.write(str(avg) + ",")

      perf_file.write("\n")
      #end for dataset 
    #end for ordering
    print_to_comp_file = False

if __name__ == "__main__":
    main()