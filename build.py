import sys
import os
import argparse
import subprocess
import glob
import re

parser = argparse.ArgumentParser()
parser.add_argument('--app', help='app <name>')
parser.add_argument('--plot', help='plot',action='store_true')
args = vars(parser.parse_args())

vector_flag='1'
delta_flag='1'


if args['app']:
  os.system('make clean')
  print 'Building system- vector:' + vector_flag + ' delta: ' + delta_flag
  CXXFLAGS='CXXFLAGS+=-DVECTORIZE='+vector_flag
  CXXFLAGS+=' CXXFLAGS+=-DDELTA='+delta_flag

  os.system('make ' + CXXFLAGS)

  print args['app']

  os.system('make ' + args['app'] + ' ' + CXXFLAGS)

if args['plot']:
  proc = subprocess.Popen(['gnuplot'], shell=True, stdin=subprocess.PIPE)
  proc.stdin.write("""set terminal png size 1000,800 enhanced font "Helvetica,20"
  set key default
  set key box

  set key samplen 2 spacing .5 font "7,7"
  set key width -4
  set key height -6
  set key at screen 0.55,screen 0.70 #for example
  """)

  size_files = glob.glob("sizes/*.dat")
  for fname in size_files:
    print fname
    matchObj = re.match( r'.*/.*_(\d+)_(.*).dat', fname, re.M|re.I)
    if matchObj:
        print(matchObj.group(1))
        t = int(matchObj.group(1))
        if t > 20:
          t = t/10
        elif t > 200:
          t = t/100
        elif t > 2000:
          t = t/1000
        elif t > 20000:
          t = t/10000
        else:
          t = 100000

        proc.stdin.write("""
          set output \"charts/size_range_"""+matchObj.group(1)+"""_max_"""+matchObj.group(2)+""".png\"
          set title \"Max Int: """+matchObj.group(2)+"""\"
          set xlabel "Array Size"
          set ylabel "Bytes Used"
          plot [:][:] '"""+fname+"""' i 0 u 1:2 w lines title columnheader(1), \
          '"""+fname+"""' i 1 u 1:2 w lines title columnheader(1), \
          '"""+fname+"""' i 2 u 1:2 w lines title columnheader(1), \
          '"""+fname+"""' i 3 u 1:2 w lines title columnheader(1), \
          '"""+fname+"""' i 4 u 1:2 w lines title columnheader(1), \
          '"""+fname+"""' i 5 u 1:2 w lines title columnheader(1)\n""")

  time_files = glob.glob("times/*.dat")
  for fname in time_files:
    matchObj = re.match( r'.*/.*_(\d+)_(.*).dat', fname, re.M|re.I)
    if matchObj:
        print(matchObj.group(1))

        t = int(matchObj.group(1))
        if t > 20:
          t = t/10
        elif t > 200:
          t = t/100
        elif t > 2000:
          t = t/1000
        elif t > 20000:
          t = t/10000
        else:
          t = 100000

        proc.stdin.write("""
          set output \"charts/time_range_"""+matchObj.group(1)+"""_max_"""+matchObj.group(2)+""".png\"
          set title \"Max Int: """+matchObj.group(2)+"""\"
          set xlabel "Array Size"
          set ylabel "Time(uSeconds)"
          plot [:][:] '"""+fname+"""' i 0 u 1:2 w lines title columnheader(1), \
          '"""+fname+"""' i 1 u 1:2 w lines title columnheader(1), \
          '"""+fname+"""' i 2 u 1:2 w lines title columnheader(1), \
          '"""+fname+"""' i 3 u 1:2 w lines title columnheader(1), \
          '"""+fname+"""' i 4 u 1:2 w lines title columnheader(1), \
          '"""+fname+"""' i 5 u 1:2 w lines title columnheader(1)\n""")


  proc.stdin.write('quit\n') #close the gnuplot window
