import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re

#special parsers
from emptyheaded.parse_output import getIInternalPerformanceInfo 
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

  datasets = ["2","2.1","2.2","2.3","2.4","2.5","2.6","2.7","2.8","2.9","3"]
  #datasets = ["1.5","2.3","3"]
  orderings = ["u_degree","u_bfs","u_shingles","u_random","u_rev_degree","u_strong_run","u_the_game"]
  layouts = ["uint"]

  perf_file = open("synth_data_order.csv", 'w')
  perf_file2 = open("synth_data_order_diff.csv", 'w')


  my_line = []
  perf_file.write("\t")
  perf_file2.write("\t")
  for dataset in datasets:
    for layout in layouts: 
      my_line.append(dataset)
  perf_file.write("\t".join(my_line))
  perf_file2.write("\t".join(my_line))
  perf_file.write("\n")
  perf_file2.write("\n\t")

  my_line = []
  my_best_dict = {}
  for ordering in orderings:
    #if(ordering != "u_the_game"):
    perf_file.write(ordering + "\t")
    for dataset in datasets:
      for layout in layouts:
        #print options.folder +"/" + dataset + "." + layout + "." + ordering +".log"
        f = open(options.folder +"/" + dataset + "." + layout + "." + ordering +".log", 'r')
        perf_info = getIInternalPerformanceInfo(f,False);
        avg = average_runs(perf_info['perf'])
        
        #print str(avg) + " " + str(best)
        my_line.append(str(avg))

        comm = '''
        if(ordering != "u_the_game"):
          if(dataset not in my_best_dict.keys() or avg < my_best_dict[dataset]):
            my_best_dict[dataset] = avg 
          my_line.append(str(avg))
        else:
          print str(avg) + " " + str(my_best_dict[dataset])
          perf_file2.write(str(float(avg/my_best_dict[dataset])) + "\t")
        '''

    perf_file.write("\t".join(my_line))
    perf_file.write("\n")
    my_line = []
    #end for dataset 
  #end for ordering

if __name__ == "__main__":
    main()
