import re

def getGaloisPerformance(f):        
  perf = []
  for line in f:
    matchObj = re.match(r'.*,.*,TotalTime,\d+,(\d+),.*', line, re.M|re.I)
    if matchObj:
      perf.append(matchObj.group(1))

  return {'perf':perf}