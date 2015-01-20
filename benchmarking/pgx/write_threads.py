import subprocess
import time
import os
import psutil
from optparse import OptionParser
import time
import re
import sys

def main():
  memfile=open("/afs/cs.stanford.edu/u/caberger/pgx-release-0.8.1/server.config" , 'w+')
  memfile.write("{\"num_workers_analysis\": "+sys.argv[1]+"}")

if __name__ == "__main__":
    main()
