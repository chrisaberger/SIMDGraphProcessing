import re

def getPostgresPerformance(f):
  result = dict()
  query = None

  for line in f:
    matchObj = re.match(r'----- (.*) -----', line, re.M|re.I)
    if matchObj:
      query = matchObj.group(1)

    matchObj = re.match(r'Time: (.*) ms', line, re.M|re.I)
    if matchObj:
        if query in result:
            result[query].append(matchObj.group(1))
        else:
            result[query] = [matchObj.group(1)]

  print result

  return result
