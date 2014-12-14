import re

def getLigraPerformance(f):        
  perf = []
  for line in f:
    matchObj = re.match(r'Running time : (\d+.\d*).*', line, re.M|re.I)
    if matchObj:
      perf.append(matchObj.group(1))

  return {'perf':perf}