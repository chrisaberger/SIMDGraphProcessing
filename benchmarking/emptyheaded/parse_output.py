import re

def getInternalPerformanceInfo(f, get_compression=False, get_bfs_details=False):
  triangle_perf = []
  bfs_perf = []
  bfs_details_timers = ["copy time", "union time", "difference", "repack"]
  bfs_details_perf = dict()
  bfs_details_perf_curr_run = dict()
  num_bytes = -1.0
  num_edges = -1.0

  for line in f:
    matchObj = re.match(r'Time\[UNDIRECTED TRIANGLE.*: (.*) s', line, re.M|re.I)
    if matchObj:
      triangle_perf.append(matchObj.group(1))

    matchObj = re.match(r'Time\[BFS.*: (.*) s', line, re.M|re.I)
    if matchObj:
      bfs_perf.append(matchObj.group(1))
      if get_bfs_details:
          # End of a run, save the sum of the times over all iterations
          for k, v in bfs_details_perf_curr_run.iteritems():
              bfs_details_perf_curr_run[k] = 0.0
              if k in bfs_details_perf:
                  bfs_details_perf[k].append(v)
              else:
                  bfs_details_perf[k] = [v]

    if get_compression:
      matchObj = re.match(r'.*Bytes.*: (\d+)', line, re.M|re.I)
      if matchObj:
        num_bytes = matchObj.group(1)

      matchObj = re.match(r'.*Number of edges.*: (\d+)', line, re.M|re.I)
      if matchObj:
        num_edges = matchObj.group(1)

    if get_bfs_details:
        for bfs_details_timer in bfs_details_timers:
            matchObj = re.match(r'Time\[' + bfs_details_timer + r'.*: (.*) s', line, re.M|re.I)
            if matchObj:
                v = float(matchObj.group(1))
                if bfs_details_timer in bfs_details_perf_curr_run:
                    bfs_details_perf_curr_run[bfs_details_timer] = bfs_details_perf_curr_run[bfs_details_timer] + v
                else:
                    bfs_details_perf_curr_run[bfs_details_timer] = v

  return {'perfs': [{'query': 'triangle_counting', 'perf': triangle_perf}, {'query': 'bfs', 'perf': bfs_perf}], 'bytes':num_bytes, 'edges':num_edges, 'bfs_details':bfs_details_perf}
