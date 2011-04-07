#!/usr/local/bin/python
#This is a script to gather all logged data and aggregate it into a single .csv file for statistical analysis

import csv
import os
import xml.dom.minidom
import glob
import sys
import re
import time
import os.path
import xml.parsers.expat
import math


# get the list of log files to process
filelist = set()
for i in range(1,len(sys.argv)):
	files = glob.glob(sys.argv[i])
	for f in files: 
		filelist.add(f)
print filelist

#specify the csv file name and initialize the top category rows 
csvlog = "rtgenstats.csv"
fopen = open(csvlog,'w')
fieldnames = ["id_src", "id_dest", "id_streamid", "cnt_ontime", "cnt_late", "cnt_miss", "cnt_dup", "time_mean", 
"time_med", "time_stddev", "time_max", "time_min", "strm_delta", "strm_buffs", "strm_count", "strm_psize"]#, "gen_conf", "rec_conf"]
csvwriter = csv.writer(fopen)
csvwriter.writerow(fieldnames)
dictwriter = csv.DictWriter(fopen, fieldnames = fieldnames)
#Variables that are needed throughout the program
id_srclist = []
id_destlist = []
id_streamidlist = []
cnt_ontimelist = []
cnt_latelist = []
cnt_misslist = []
cnt_duplist = []
strm_buffslist = []
strm_countlist = []
srtm_psizelist = []
strm_deltalist = []
strm_psizelist = []
time_meanlist = []
time_stddevlist = []
mmedianslist = []
minlist = []
maxlist = []
generatoridlist = []
receiveridlist = []
generatorlist = []
receiverlist = []
differentiatelist = []
streamstartlist = []
totalstreams = 0
generatorfiles = 0
receiverfiles = 0

for f in filelist: 
	#initialize variables
	streamnum = 0
	localdifferentiatelist = []
	#Get the stream id and receiver tag information. This is used to differentiate between generators and receivers. 
	#It may seem backwards, but generators have a "receiver=" tag, while receivers have a "sender=" tag, so by looking at the receiver tag info, one 	 #can tell if the stream is a generator or receiver. This information is put into lists and then sorted by stream id to keep it in order.
	print "processing "
	doc = xml.dom.minidom.parse(f)
	for node in doc.getElementsByTagName("streamstart"):
		differentiate = [node.getAttribute("receiver"), node.getAttribute("streamid")]
		differentiatelist.append(differentiate)
		localdifferentiatelist.append(differentiate)
	for node in differentiatelist:
		streamnum = streamnum + 1
	for node in localdifferentiatelist:
		if(node[0] == ""):
			receiverfiles = receiverfiles + 1
		else:
			generatorfiles = generatorfiles + 1
#print differentiatelist
#print receiverfiles
#print generatorfiles
differentiatelist.sort(key = lambda differentiatelist:differentiatelist[1])
#print differentiatelist
#get info out of the streamstarts and put them into lists for further processing
for f in filelist:	
	print "more processing"
	doc = xml.dom.minidom.parse(f)
	for node in doc.getElementsByTagName("streamstart") : 
		streamstarts = [node.getAttribute("sender"), node. getAttribute("host"), node.getAttribute("streamid"), 
		node.getAttribute("fseqnum"), node.getAttribute("proctime"), node.getAttribute("initbuffs"), node.getAttribute("packetsize")]
		streamstartlist.append(streamstarts)
streamstartlist.sort(key = lambda streamstartlist:streamstartlist[2])
#print streamnum
#print streamstartlist
ontime = -1
late = 0
missed = 0
duplicate = -1
index = 0
n = 0
packnum = 0
x = 0
y = 0
i = 0
pack = 0
lpacknum = 0
lhours = 0	
lmins = 0
lsecs = 0
lmicrosecs = 0
streamidlist = []
streamstoplist = []
packetlist = []	
biglist = []
rowlist = []
packetnumlist = []
packettimelist = []
microseclist = []
packetnumlist = []
timelist = []	
streamlist = []
interarrivaltimelist = []
timinglist = []
timestreamlist = []
interarrivelist = []
istrmcountlist = []
fullcountlist = []
genidlist = []
medianlist = []	
receiverstrmlist = []
generatorstrmlist = []
s = 0	
#print len(streamstartlist)
print "Generator files: ",generatorfiles
print "Receiver files: ",receiverfiles
while(s<streamnum):		
	for node in streamstartlist:
		#print node
		stcount = node[3]
		#used to get int indexes
		estrmcount = stcount.encode("utf-8")
		istrmcount = int(estrmcount)
		if(differentiatelist[s][0] == ""):
			istrmcountlist.append(istrmcount)
			streamcountnum = istrmcountlist[0]
		#print node[0]
		if(node[0] != ""):
			receiverstrmlist.append(node)
		else:
			generatorstrmlist.append(node)
		s = s + 1
for node in receiverstrmlist:
	count = streamnum	
	id_src = node[0]
	id_dest = node[1]
	id_streamid = node[2]
	strm_buffs = node[5]
	strm_count = node[3]
	strm_delta = node[4]
	strm_psize = node[6]
	id_srclist.append(id_src)
	id_destlist.append(id_dest)
	id_streamidlist.append(id_streamid)
	streamidlist.append(id_streamid)
	strm_buffslist.append(strm_buffs)
	strm_countlist.append(strm_count)
	strm_deltalist.append(strm_delta)
	strm_psizelist.append(strm_psize)	
#print differentiatelist[2][0]
#print istrmcountlist
print receiverstrmlist
print generatorstrmlist
r = 0
t = 0
gen_conflist = []
rec_conflist = []
genonlylist = []
gen_rec_conflist = []
#print receiverstrmlist[0]
print len(generatorstrmlist)
print len(receiverstrmlist)
while(r != len(generatorstrmlist)):
	#print r
	#print t
	if(len(receiverstrmlist) == 0):
		print "Generator file confirmation for stream id: ", generatorstrmlist[r][2], "**NO RECEIVER FILE**"
		genonlylist.append(generatorstrmlist[r][2])
		r = r + 1
		if(r == len(generatorstrmlist)):
			break
	if(len(receiverstrmlist) != 0):
		if(generatorstrmlist[r][2] == receiverstrmlist[t][2]):
			genconf = [1, generatorstrmlist[r][2]]
			#gen_conflist.append(genconf)
			#rec_conflist.append(genconf)
			print "Generator and receiver file confirmation for stream id: ", generatorstrmlist[r][2]
			gen_rec_conflist.append(generatorstrmlist[r][2])
			r = r + 1
			t = 0
			if(r == len(generatorstrmlist)):
				break
		if(t == (len(receiverstrmlist) - 1)):
			print "Generator file confirmation for stream id: ", generatorstrmlist[r][2], "**NO RECEIVER FILE**"
			genonlylist.append(generatorstrmlist[r][2])
			r = r + 1
			t = 0
			if(r == len(generatorstrmlist)):
				break
		if(generatorstrmlist[r][2] != receiverstrmlist[t][2]):
			t = t + 1
	
	#if(r == len(generatorstrmlist) or t == len(receiverstrmlist)):
		#break
print genonlylist	
	
	#else:
	#	genconf = [1, generatorstrmlist[r][2]]
	#	recconf = [1, receiverstrmlist[t][2]]	
	#	gen_conflist.append(genconf)
	#	rec_conflist.append(recconf)
	#t = t + 1
	#if(t >= len(receiverstrmlist)):
		#genconf = [0, generatorstrmlist[r][2]]
		#gen_conflist.append(genconf)
	#	recconf = [1, receiverstrmlist[t][2]]
	#	rec_conflist.append(recconf)
	#	print "Generator file confirmation for stream id: ", generatorstrmlist[r][2], "**no receiver file**"
print gen_conflist
print rec_conflist	
		
num = 0
#This adds the number of packets in each stream together consecutively - found that this was needed for the mean operation	
for node in istrmcountlist:
	if(len(istrmcountlist) == 1):
		fullcountlist.append(streamcountnum)
		break
	fullcountlist.append(streamcountnum)
	#print istrmcountlist[num + 1]
	#print fullcountlist[num]
	streamcountnum = fullcountlist[num] + istrmcountlist[num + 1]
	num = num + 1
	if(num == len(istrmcountlist) - 1):
		fullcountlist.append(streamcountnum)
		break
#print fullcountlist
halfullcountlist = []	
i = 0
#This gets a list of numbers used for the indexs of the middle of the lists - helps get the median further on
for node in istrmcountlist:
	halfullcountlist.append(node/2)

#print halfullcountlist
for f in filelist:
	index = 0
	print "further processing"
	#gather the data from the packets section of the .log file
	doc = xml.dom.minidom.parse(f)
	for node in doc.getElementsByTagName("packet") :	
		packets = [node.getAttribute("streamid"), node.getAttribute("seqnum"), node.getAttribute("status"), 
		node.getAttribute("date"), node.getAttribute("time")]
		packetlist.append(packets)
	packetlist.sort(key = lambda packetlist:packetlist[0])
#print packetlist
#print streamstartlist
#print streamstartlist[index][2]
	
#first attempt at count information: get all packets that are ontime, missed or late for the entire file, not individual streams
#starts at the first stream in the sorted list
while (index != receiverfiles):
	for node in packetlist:
		#packets are ontime, late or missed
		if(node[0] == receiverstrmlist[index][2]):
			if(node[2] == "ontime"):
				ontime = ontime + 1
			if(node[2] == "late"):
				late = late + 1
			if(node[2] == "missed"):
				missed = missed + 1
			#duplicate packets
			if(node[1] == packetlist[n][1]):
				duplicate = duplicate + 1
	count = count - 1
	index = streamnum - count
	n = n + 1
	#Move all ontime, late, and missed packet information into lists
	cnt_ontimelist.append(ontime)
	cnt_latelist.append(late)
	cnt_misslist.append(missed)
	cnt_duplist.append(duplicate)
	#reset values
	ontime = -1
	late = 0
	missed = 0
	duplicate = -1

#time information
for node in packetlist:
	#convert time and packet numbers into long from unicode so that mathematical operations can be performed on them
	streamid = node[0]
	packnum = node[1]
	hours = node[4][0:2]	
	mins = node[4][3:5]
	secs = node[4][6:8]
	microsecs = node[4][9:16]
	estreamid = streamid.encode("utf-8")
	epacknum = packnum.encode("utf-8")
	ehours = hours.encode("utf-8") 
	emins = mins.encode("utf-8")
	esecs = secs.encode("utf-8")
	emicrosecs = microsecs.encode("utf-8")
	lstreamid = long(estreamid)
	lpacknum = long(epacknum)	
	lhours = long(ehours) 
	lmins = long(emins)
	lsecs = long(esecs)
	lmicrosecs = long(emicrosecs)
	hoursinmicrosecs = lhours * 3600000000
	minsinmicrosecs = lmins * 60000000
	secsinmicrosecs = lsecs * 1000000
	totaltimeinmicrosecs = hoursinmicrosecs + minsinmicrosecs + secsinmicrosecs + lmicrosecs
	streamlist.append(streamid)
	microseclist.append(totaltimeinmicrosecs)
	packetnumlist.append(lpacknum)
#list of stream ids, packet numbers, and total microseconds
timelist.append(streamlist)
timelist.append(packetnumlist)
timelist.append(microseclist)
#print len(timelist[1])
#print id_streamidlist
#print timelist
index = 0
place = 0	
#print streamnum

#split the timelist into a list of stream ids and the interarrival time between packets	
while(index != streamnum):
	#print index
	if(len(timelist[0]) == 0):
		break
	#if the end of the list is reached, end the while loop			
	if((len(timelist[1])-1) == place):
		#print len(timelist[1]) -1
		index = index + 1
		break					
	#checking that streamids are the same
	if(timelist[0][place] == streamidlist[index]):
		#print id_streamidlist[index]
		#check to see if packets are consecutive
		if((timelist[1][place + 1] - timelist[1][place]) != 1):
			place = place + 1
			#print place
			#print interarrivaltime
			#print (timelist[1][place + 1] - timelist[1][place])
		#if packets are consecutive, get the interarrival time				
		else:	
			place = place + 1 							
			interarrivaltime = (timelist[2][place] - timelist[2][place - 1])
			streamid = timelist[0][place]
			timestreamlist.append(streamid)				
			interarrivaltimelist.append(interarrivaltime)		
	else:
		index = index + 1							
timinglist.append(timestreamlist)
timinglist.append(interarrivaltimelist)
#print timinglist
#once the timinglist is created, separate the interarrival times by stream id so that mean, standard devation and median can be calculated 		#according to stream id
count = 0
interarrive = 0	
i = 0
#cycle through the entire list
#MEAN
#print len(timinglist[0])
#print fullcountlist
while(count < len(timinglist[0])):
	interarrive = interarrive + timinglist[1][count]
	count = count + 1
	#if the count is equal to the end of a stream(the last packet in the stream)
	if(count == fullcountlist[i]): 
		time_mean = 0
		time_mean = (interarrive/istrmcountlist[i])
		time_meanlist.append(time_mean)
		i = i + 1
		interarrive = 0 
#print time_meanlist
count = 0
i = 0
tot = 0
#STDDEV is wrong
#print fullcountlist
#print fullcountlist[0]
#print fullcountlist[1]
#print len(timinglist[1])
#print timinglist[1]
#print time_meanlist
while(count < len(timinglist[0])):
	#print timinglist[1][count]
	#print time_meanlist[i]
	#print i
	#print len(timinglist[0])
	#print count
	#print len(time_meanlist)
	diff = timinglist[1][count] - time_meanlist[i]
	#print diff
	squared = (diff*diff)
	#print squared
	tot = tot + squared
	#print count 
	count = count + 1
	#print fullcountlist[i]
	if(count == fullcountlist[i]):
		#print tot
		#print count
		div = (tot/count)
		#print div
		time_stddev = math.sqrt(div)
		#print time_stddev
		time_stddevlist.append(time_stddev)
		#print i
		tot = 0
		i = i + 1
if(len(timinglist[0]) != 0):
	timinglist.sort(key = lambda timinglist:timinglist[0])####problem line with the gen/rec confirmation
#print timinglist
i = 0
count = 0
begin = 0
#print fullcountlist
#MEDIAN, MIN, MAX
while(count < len(timinglist[0])):
	medianlist.append(interarrivaltimelist[count])
	count = count + 1
	if(count == fullcountlist[i]):
		#print medianlist
		medianlist.sort(key = lambda medianlist:medianlist)
		#print medianlist
		mmedianslist.append(medianlist[halfullcountlist[i]])
		minlist.append(medianlist[begin])
		maxlist.append(medianlist[len(medianlist) - 1])
		#print mmedianslist
		#print minlist
		#print maxlist
		medianlist = []
		i = i + 1
x = x + 1
#print streamstartlist

#		for node in doc.getElementsByTagName("sstart") :	
#			streamid = node.getAttribute("streamid")
#			genidlist.append(streamid)
#			print genidlist
#		genidlist.sort(key = lambda gen_conflist:gen_conflist[0])

for node in genonlylist:
	id_srclist.append(-1)	
	id_destlist.append(-1)
	id_streamidlist.append(node)
	cnt_ontimelist.append(-1)
	cnt_latelist.append(-1)
	cnt_misslist.append(-1)
	cnt_duplist.append(-1)
	strm_buffslist.append(-1)
	strm_countlist.append(-1)
	strm_deltalist.append(-1)
	strm_psizelist.append(-1)
	time_meanlist.append(-1)
	time_stddevlist.append(-1)
	mmedianslist.append(-1)
	minlist.append(-1)
	maxlist.append(-1)	

#This puts all of the individual data lists into a bigger list
biglist.append(id_srclist)
biglist.append(id_destlist)
biglist.append(id_streamidlist)
biglist.append(cnt_ontimelist)
biglist.append(cnt_latelist)
biglist.append(cnt_misslist)
biglist.append(cnt_duplist)
biglist.append(strm_buffslist)
biglist.append(strm_countlist)	
biglist.append(strm_deltalist)	
biglist.append(strm_psizelist)
biglist.append(time_meanlist)
biglist.append(time_stddevlist)
biglist.append(mmedianslist)
biglist.append(minlist)
biglist.append(maxlist)
#biglist.append(gen_conflist[0])
#biglist.append(rec_conflist)
print biglist
#print len(biglist)
#print streamnum
#print len(biglist[0])
#print len(biglist)
x = 0
#Takes the data from big list and puts it into the rowlist(each stream's data will finally then be all together in a list)		
#The rowlist is written into a .csv file using dictwriter
#This is repeated for the number of streams in the .log file
while(x < (streamnum - len(gen_rec_conflist))):		
	for node in biglist:	
		#print node[x]
		rowlist.append(node[x])
	rowdict = {"id_src": rowlist[0], "id_dest": rowlist[1], "id_streamid": rowlist[2], "cnt_ontime": rowlist[3], 
	"cnt_late": rowlist[4], "cnt_miss": rowlist[5], "cnt_dup": rowlist[6], "strm_buffs": rowlist[7], "strm_count": rowlist[8], 		"strm_delta": rowlist[9], "strm_psize": rowlist[10],
 	"time_mean": rowlist[11], "time_stddev": rowlist[12], "time_med": rowlist[13], 
	"time_min": rowlist[14], "time_max": rowlist[15]}#, "gen_conf": rowlist[16], "rec_conf": rowlist[17]}
	dictwriter.writerow(rowdict)
	rowlist = []
	x = x + 1
	#print x	
	
		
fopen.close()

