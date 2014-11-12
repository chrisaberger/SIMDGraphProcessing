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
    help="[REQUIRED] input edge list to parse")

  parser.add_option("-t", "--threads", dest="threads",
    help="[REQUIRED] number of threads to run with")

  parser.add_option("-r", "--runs", dest="runs",
    help="[REQUIRED] number of runs for each app")

  parser.add_option("-p", "--poll", dest="poll",
    help="[REQUIRED] time slice for polling")

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

  systems = ['SpaceGraph']
  apps = ['triangle_counting']

  #only way (i could get to work) to control threading in these systems
  os.environ["GRAPHLAB_THREADS_PER_WORKER"]=options.threads
  os.environ["OMP_NUM_THREADS"]=options.threads

  memfile=open("/afs/cs.stanford.edu/u/caberger/pgx-release-0.8.1/server.config" , 'w+')
  memfile.write("{\"num_workers_analysis\": "+options.threads+"}")

  system_dictionary = { \
    'SpaceGraph': { \
      'dir' : '/afs/cs.stanford.edu/u/caberger/SpaceGraph', \
      'triangle_counting':{ \
        'args':["/afs/cs.stanford.edu/u/caberger/SpaceGraph/bin/undirected_triangle_counting",options.dataset + "/bin/udata.bin",options.threads,"hybrid"] \
      } \
    }, \
    'GraphLab': { \
      'dir':'/afs/cs.stanford.edu/u/caberger/graphlab/', \
      'triangle_counting':{ \
        'args':["/afs/cs.stanford.edu/u/caberger/graphlab/release/toolkits/graph_analytics/undirected_triangle_count","--graph="+options.dataset + "/glab_undirected/data.txt","--format=snap","--ncpus="+options.threads] \
      } \
    }, \
    'pgx': { \
      'dir' : '/afs/cs.stanford.edu/u/caberger/pgx-release-0.8.1', \
      'triangle_counting':{ \
        'args':["java","-cp","/afs/cs.stanford.edu/u/caberger/pgx-release-0.8.1/lib/*:/afs/cs.stanford.edu/u/caberger/pgx-release-0.8.1/third-party/*:/afs/cs.stanford.edu/u/caberger/pgx-release-0.8.1/classes","demos.UndirectedTriangleCounting",options.dataset+"/pgx/sample.edge.json"] \
      } \
    }, \
    'GraphChi': { \
      'dir': '/afs/cs.stanford.edu/u/caberger/graphchi-cpp', \
      'triangle_counting':{ \
        'args':["/afs/cs.stanford.edu/u/caberger/graphchi-cpp/bin/example_apps/trianglecounting","file",options.dataset+"/edgelist/data.txt","filetype","edgelist","execthreads",options.threads,"loadthreads",options.threads,"niothreads",options.threads,"--nshards=2"] \
      } \
    }, \
    'snap': { \
      'dir': '/afs/cs.stanford.edu/u/caberger/snapr', \
      'triangle_counting':{ \
        'args':["/afs/cs.stanford.edu/u/caberger/snapr/examples/tcount/centrality","-i:"+options.dataset+"/edgelist/data.txt",options.threads] \
      } \
    }, \

  } \

  outpath = "/dfs/scratch0/caberger/run_outputs/" + options.dataset.split('/').pop()
  if not os.path.exists(outpath):
    os.system('mkdir ' + outpath)
  
  for system in systems:
    os.chdir(system_dictionary[system]['dir'])
    for app in apps:
      new_path = outpath + '/' + app;

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
        SLICE_IN_SECONDS = 1
        p = psutil.Popen(system_dictionary[system][app]['args'],stdout=stdfile)
        memlists_i = 0
        while p.poll() is None:
          comm='''
          if memlists_i >= len(memlists):
            memlists.append(p.memory_info().rss)
          else:
            memlists[memlists_i] += p.memory_info().rss
          memlists_i += 1
          '''
          time.sleep(SLICE_IN_SECONDS)

      comm = '''
      for mem in memlists:
        memfile.write(str(float(mem)/float(options.runs)) + ',')
        '''

if __name__ == "__main__":
    main()