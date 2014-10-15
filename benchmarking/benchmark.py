import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re

def parseInput():
  parser = OptionParser()
  parser.add_option("-e", "--dataset", dest="dataset",
    help="[REQUIRED] input edge list to parse", metavar="FILE")

  parser.add_option("-t", "--threads", dest="threads",
    help="[REQUIRED] number of threads to run with", metavar="FILE")

  parser.add_option("-r", "--runs", dest="runs",
    help="[REQUIRED] number of runs for each app", metavar="FILE")

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

  system_dictionary = { \
    'SpaceGraph': { \
      'triangle_counting':{ \
        'args':["/afs/cs.stanford.edu/u/caberger/SpaceGraph/bin/undirected_triangle_counting",options.dataset + "/edgelist/data.txt",options.threads,"hybrid"] \
      } \
    } \
  } \

  outpath = "/lfs/raiders3/0/caberger/run_outputs/" + options.dataset.split('/').pop()
  if not os.path.exists(outpath):
    os.system('mkdir ' + outpath)
  
  for system in system_dictionary:
    for apps in system_dictionary[system]:
      new_path = outpath + '/' + apps;

      if not os.path.exists(new_path):
        os.system('mkdir ' + new_path)
      new_path += '/' + system
      if not os.path.exists(new_path):
        os.system('mkdir ' + new_path)
      new_path += '/' + time.strftime("%m-%d")
      if not os.path.exists(new_path):
        os.system('mkdir ' + new_path)

      new_path += '/' + options.threads + "t_" + time.strftime("%H-%M-%S")
      os.system('mkdir ' + new_path)

      memfile=open(new_path + "/avg_mem_usage.csv" , 'w+')
      memlists = []
      for i in range(0,int(options.runs)):
        stdfile=open(new_path + "/stdout_"+str(i)+".txt" , 'w+')
        SLICE_IN_SECONDS = 0.01
        p = psutil.Popen(system_dictionary[system][apps]['args'],stdout=stdfile)
        memlists_i = 0
        while p.poll() is None:
          if memlists_i >= len(memlists):
            memlists.append(p.memory_info().rss)
          else:
            memlists[memlists_i] += p.memory_info().rss
          memlists_i += 1
          time.sleep(SLICE_IN_SECONDS)

      for mem in memlists:
        memfile.write(str(float(mem)/float(options.runs)) + ',')


if __name__ == "__main__":
    main()