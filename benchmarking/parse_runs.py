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
from graphlab.parse_output import getGraphLabPerformance
from galois.parse_output import getGaloisPerformance
from ligra.parse_output import getLigraPerformance

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

  #datasets = ["california","higgs","flickr","socLivejournal","orkut","cid-patents","pokec","twitter2010","wikipedia"]
  datasets = ["baidu", "twitter2010", "wikipedia"]

  threads = ["1","24","48"]

  print options.folder
  matchObj = re.match(r'/dfs/scratch0/\w*/output/(\w*)_.*', options.folder, re.M|re.I)
  system = matchObj.group(1)
  print system

  for dataset in datasets:
    for thread in threads:
      f = open(os.path.join(options.folder, system + "." + dataset + "." + thread + ".log"))
      if system == "graphlab":
        perf_info = getGraphLabPerformance(f);
        avg = average_runs(perf_info['perf'])
        print "data: " + dataset + " t: " + thread + " time: " + str(avg)
      elif system == "galois":
        perf_info = getGaloisPerformance(f);
        avg = average_runs(perf_info['perf'])
        print "data: " + dataset + " t: " + thread + " time: " + str(avg)
      elif system == "ligra":
        perf_info = getLigraPerformance(f);
        avg = average_runs(perf_info['perf'])
        print "data: " + dataset + " t: " + thread + " time: " + str(avg)
      elif system == "emptyheaded":
        perf_info = getInternalPerformanceInfo(f);
        for p in perf_info['perfs']:
            query = p['query']
            avg = average_runs(p['perf'])
            print "query: " + p['query'] + " data: " + dataset + " t: " + thread + " time: " + str(avg)

if __name__ == "__main__":
    main()
