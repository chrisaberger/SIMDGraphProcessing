import re

def getInternalPerformanceInfo(f,get_compression=False):        
  perf = []
  num_bytes = -1.0
  num_edges = -1.0

  for line in f:
    matchObj = re.match(r'Time\[UNDIRECTED TRIANGLE.*: (.*) s', line, re.M|re.I)
    if matchObj:
      perf.append(matchObj.group(1))

    if get_compression:
      matchObj = re.match(r'.*Bytes.*: (\d+)', line, re.M|re.I)
      if matchObj:
        num_bytes = matchObj.group(1)
      
      matchObj = re.match(r'.*Number of edges.*: (\d+)', line, re.M|re.I)
      if matchObj:
        num_edges = matchObj.group(1)

  return {'perf':perf, 'bytes':num_bytes ,'edges':num_edges }