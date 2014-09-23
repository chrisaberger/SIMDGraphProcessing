import sys
import os
import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--app', help='app <name>')
parser.add_argument('--plot', help='plot <name>')
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
  set output "chart_2.png"
  set key default
  set key box

  set key samplen 2 spacing .5 font "7,7"
  set key width -4
  set key height -6
  set key at screen 0.55,screen 0.70 #for example
  set xtics 200000
  """)
  #set key at screen 0.85,screen 0.22 #for example

  proc.stdin.write("""
    set title "Plots1"
    set xlabel "Range"
    set ylabel "Bytes Used"
    plot [:][:] '"""+args['plot']+"""' i 0 u 1:2 w lines title columnheader(1), \
    '"""+args['plot']+"""' i 1 u 1:2 w lines title columnheader(1), \
    '"""+args['plot']+"""' i 2 u 1:2 w lines title columnheader(1), \
    '"""+args['plot']+"""' i 3 u 1:2 w lines title columnheader(1), \
    '"""+args['plot']+"""' i 4 u 1:2 w lines title columnheader(1), \
    '"""+args['plot']+"""' i 5 u 1:2 w lines title columnheader(1)\n""")
  

  proc.stdin.write('quit\n') #close the gnuplot window
