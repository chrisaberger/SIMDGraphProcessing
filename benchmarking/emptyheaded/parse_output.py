import re

def getInternalPerformanceInfo(f,get_compression=False):
  triangle_perf = []
  bfs_perf = []
  num_bytes = -1.0
  num_edges = -1.0

  for line in f:
    matchObj = re.match(r'Time\[UNDIRECTED TRIANGLE.*: (.*) s', line, re.M|re.I)
    if matchObj:
      triangle_perf.append(matchObj.group(1))

    matchObj = re.match(r'Time\[BFS.*: (.*) s', line, re.M|re.I)
    if matchObj:
      bfs_perf.append(matchObj.group(1))

    if get_compression:
      matchObj = re.match(r'.*Bytes.*: (\d+)', line, re.M|re.I)
      if matchObj:
        num_bytes = matchObj.group(1)

      matchObj = re.match(r'.*Number of edges.*: (\d+)', line, re.M|re.I)
      if matchObj:
        num_edges = matchObj.group(1)

  return {'perfs': [{'query': 'triangle_counting', 'perf': triangle_perf}, {'query': 'bfs', 'perf': bfs_perf}], 'bytes':num_bytes ,'edges':num_edges}
