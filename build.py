import sys
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--app', help='app <name>', required=True)
args = vars(parser.parse_args())

vector_flag='1'
delta_flag='1'

os.system('make clean')
print 'Building system- vector:' + vector_flag + ' delta: ' + delta_flag
CXXFLAGS='CXXFLAGS+=-DVECTORIZE='+vector_flag
CXXFLAGS+=' CXXFLAGS+=-DDELTA='+delta_flag

os.system('make ' + CXXFLAGS)

print args['app']

os.system('make ' + args['app'] + ' ' + CXXFLAGS)