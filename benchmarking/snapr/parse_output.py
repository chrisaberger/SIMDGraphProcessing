import re

def getSnaprPerformance(f):        
  perf = []
  num_bytes = -1.0
  num_edges = -1.0

  for line in f:
    matchObj = re.match(r'Time\[Triangle Counting\]: (.*) s', line, re.M|re.I)
    if matchObj:
      perf.append(matchObj.group(1))

  return {'perf':perf, 'bytes':num_bytes ,'edges':num_edges }