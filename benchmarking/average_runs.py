def average_runs(perf):
  if len(perf) == 0:
    avg = -1.0
  elif len(perf) < 3:
    avg = 0.0
    for time in perf:
      avg += float(time)
    avg /= len(perf)
  else:
    avg = 0.0
    perf.sort()
    perf.pop()
    perf.remove(perf[0])
    for time in perf:
      avg += float(time)
    avg /= len(perf)
  return avg