import fileinput
import re
import operator

def printStatsOnList(listIn,name):
	listIn.sort(key=operator.itemgetter('time'))
	[listInsum,listInmin,listInmax] = sum(item['time'] for item in listIn),min(item['time'] for item in listIn),max(item['time'] for item in listIn)
	[listInin,listInout] = (sum(item['i1'] for item in listIn)+sum(item['i2'] for item in listIn))/(2*len(listIn)),sum(item['o'] for item in listIn)/len(listIn)
	print name + ' - Total time : ' + str(listInsum) + 'ms'
	print name + ' - AverageTime : ' + str(listInsum/ len(listIn)) + 'ms' + ' WorstTime: ' + str(listInmax) + 'ms BestTime: ' + str(listInmin) + "ms #Occurs: " + str(len(listIn))
	print name + ' - AverageOutSetSize: ' + str(listInout) + ' AverageInSetSize: ' + str(listInin)
	print '\n'

def printDatOnList(listIn,fileIn1,fileIn2):
    for item in listIn:
        fileIn1.write(str(item['i1']) + ' ' + str(item['i2']) + ' ' + str(item['time']) + '\n')
        outputRate = item['o'] if(item['time']==0) else str(float(item['o'])/item['time'])
        fileIn2.write(str(item['i1']) + ' ' + str(item['i2']) + ' ' + str(outputRate) + '\n')

l1 = []
l2 = []
l3 = []
fileIn = fileinput.input()
for line in fileIn:
    matchObj = re.match('(\d) - (\w+): *',line)
    if matchObj:
    	if(int(matchObj.group(1)) == 1):
    		myL = l1
    	elif(int(matchObj.group(1)) == 2):
    		myL = l2
    	elif(int(matchObj.group(1)) == 3):
    		myL = l3
    	
    	if(matchObj.group(2)=='time'):
    		matchObj2 = re.match('\d - \w+: (\d+.\d+)',line)
    		line = fileIn.readline()
    		matchObj3 = re.match('\d - \w+: (\d+) \w+: (\d+) \w+: (\d+)',line)
    		myL.append({'time':float(matchObj2.group(1)),'i1':int(matchObj3.group(1)),'i2':int(matchObj3.group(2)),'o':int(matchObj3.group(3))})

    	if(int(matchObj.group(1)) == 1): 
    		l1 = myL
    	elif(int(matchObj.group(1)) == 2):
    		l2 = myL 
    	elif(int(matchObj.group(1)) == 3):
    		l3 = myL
fo = open("inputSet.dat", "w+")
fo2 = open("outputRate.dat", "w+")
if(len(l1) > 0):
    printStatsOnList(l1,"BS AND BS")
    printDatOnList(l1,fo,fo2)
if(len(l2) > 0):
    printStatsOnList(l2,"CSR probe BS")
    printDatOnList(l2,fo,fo2)
if(len(l3) > 0):
    printStatsOnList(l3,"CSR Intersect")
    printDatOnList(l3,fo,fo2)
fo.close()
fo2.close()