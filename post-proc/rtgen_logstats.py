#!/usr/local/bin/python
#This is a script to gather all logged data and aggregate it into a single .csv file for statistical analysis.
#This will gather data from any number of generator.log and receiver.log files in the current directory.

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

#Get the list of .log files to process
filelist = set()
for i in range(1,len(sys.argv)):
	files = glob.glob(sys.argv[i])
	for f in files: 
		filelist.add(f)
print filelist

#Specify the csv file name and initialize the top category rows that the dictwriter will write to at the end
csvlog = "rtgenstats.csv"
fopen = open(csvlog,'w')
fieldnames = ["id_src", "id_dest", "id_streamid", "cnt_ontime(packets)", "cnt_late(packets)", "cnt_miss(packets)", "cnt_dup(packets)", "time_mean(microsec)", "time_med(microsec)", "time_stddev(microsec)", "time_max(microsec)", "time_min(microsec)", "strm_delta(millisecs)", "strm_buffs(packets)", "strm_count(packets)", "strm_psize(bytes)"]
csvwriter = csv.writer(fopen)
#Write the fieldnames to the .csv file
csvwriter.writerow(fieldnames)
dictwriter = csv.DictWriter(fopen, fieldnames = fieldnames)

#Variables and lists that are needed throughout the entire program
id_srclist = [] 	#source of the stream(ip address for receivers, hostname for generators)
id_destlist = []	#destination of the stream(hostname for receivers, ip address for generators)
id_streamidlist = []	#stream ids
cnt_ontimelist = []	#number of ontime packets
cnt_latelist = []	#number of late packets
cnt_misslist = []	#number of missed packets
cnt_duplist = []	#number of duplicate packets	
strm_buffslist = []	#initial buffer size
strm_countlist = []	#number of packets in a stream	
strm_deltalist = []	#time between each packet (milliseconds)
strm_psizelist = []	#size of each packet
time_meanlist = []	#average interarrival time
time_stddevlist = []	#standard deviation of the interarrival times
mmedianslist = []	#median of interarrival times
minlist = []		#minimum packet interarrival time (microseconds) 
maxlist = []		#maximum packet interarrival time (microseconds)	
generatoridlist = []	#generator stream ids
receiveridlist = []	#receiver stream ids
differentiatelist = []	#list of "receiver=" tag attribute and "streamid=" tag attribute
streamstartlist = []	#list of "sender=", "host=", "streamid=", "fseqnum=", "proctime=", "initbuffs=", "packetsize=", "hostname=" and "receiver=" tags 
generatorfiles = 0	#number of generator files
receiverfiles = 0	#number of receiver files	

#Get the stream id and receiver tag information. This is used to differentiate between generator files and receiver files. It may seem backwards, but generators have a "receiver=" tag, while receivers have a "sender=" tag, so by looking at the receiver or sender tag (receiver tag used in this program) info, one can tell if the stream is a generator or receiver. This information is put into lists and then sorted by stream id to keep it in order.
print "processing "
for f in filelist: 
	streamnum = 0
	localdifferentiatelist = []#used only to find the number of generator and receiver files
	doc = xml.dom.minidom.parse(f)
	for node in doc.getElementsByTagName("streamstart"):
		differentiate = [node.getAttribute("receiver"), node.getAttribute("streamid")]
		differentiatelist.append(differentiate)
		localdifferentiatelist.append(differentiate)
	for node in differentiatelist:
		streamnum = streamnum + 1
	#separate the files into receiver fils and generator files
	for node in localdifferentiatelist:
		if(node[0] == ""):
			receiverfiles = receiverfiles + 1
		else:
			generatorfiles = generatorfiles + 1
differentiatelist.sort(key = lambda differentiatelist:differentiatelist[1])

#Gathering the streamstart tag data into a list
for f in filelist:	
	#print "more processing"
	doc = xml.dom.minidom.parse(f)
	for node in doc.getElementsByTagName("streamstart") : 
		streamstarts = [node.getAttribute("sender"), node. getAttribute("host"), node.getAttribute("streamid"), 
		node.getAttribute("fseqnum"), node.getAttribute("proctime"), node.getAttribute("initbuffs"), node.getAttribute("packetsize"),
		node.getAttribute("hostname"), node.getAttribute("receiver")]
		streamstartlist.append(streamstarts)
streamstartlist.sort(key = lambda streamstartlist:streamstartlist[2])

#Timing variables and lists
ontime = 0
late = 0
missed = 0
duplicate = 0
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
streamidlist = []		#list of stream ids	
packetlist = []			#list of "streamid=", "seqnum=", "status=", "date=" and "time=" tags inside the packet header of a (receiver) log file
biglist = []			#final list of all individual data lists
rowlist = []			#used at the end to differentiate data into rows
packetnumlist = []		#packet numbers in long instead of unicode		
microseclist = []		#list of the total time in microseconds of packet's arrival time
timelist = []			#list of stream ids, packet numbers, and total microseconds
streamlist = []			#list of streams
interarrivaltimelist = []	#list of interarrival times between packets
timinglist = []			#list of stream ids and the interarrival time for the packets
timestreamlist = []		#list of stream ids that is used in the timinglist
istrmcountlist = []		#the number of packets in a stream in int instead of unicode, individually, not added together
fullcountlist = []		#the number of packets consecutively added together, just istrmcountlist added upon itself
medianlist = []			#a sorted list of interarrival times, used to find the median, max and min
receiverstrmlist = []		#list of receiver stream ids
generatorstrmlist = []		#list of generator stream ids
s = 0	
#Print number of generator files
print "Generator files: ",generatorfiles
print "Receiver files: ",receiverfiles
#Move the generator streams and receiver streams into separate lists
while(s<streamnum):		
	for node in streamstartlist:
		stcount = node[3]
		#used to get int indexes
		estrmcount = stcount.encode("utf-8")
		istrmcount = int(estrmcount)
		if(differentiatelist[s][0] == ""):
			istrmcountlist.append(istrmcount)
			streamcountnum = istrmcountlist[0]
		if(node[0] != ""):
			receiverstrmlist.append(node)
		else:
			generatorstrmlist.append(node)
		s = s + 1

#Operate only the receiver streams
for node in receiverstrmlist:
	count = streamnum	
	id_src = node[0]
	id_dest = node[1]
	id_streamid = node[2]
	strm_buffs = node[5]
	strm_count = node[3]
	estrm_count = strm_count.encode("utf-8")
	lstrm_count = long(estrm_count)
	strm_delta = node[4]
	strm_psize = node[6]
	id_srclist.append(id_src)
	id_destlist.append(id_dest)
	id_streamidlist.append(id_streamid)
	streamidlist.append(id_streamid)
	strm_buffslist.append(strm_buffs)
	strm_countlist.append(lstrm_count + 1)
	strm_deltalist.append(strm_delta)
	strm_psizelist.append(strm_psize)	

#Generator variables and lists
r = 0
t = 0	
genonlystrmlist = []		#list of generator file streams
genonlyhostlist  = []		#list of generator file hosts
genonlyreceiverlist = []	#list of generator file receivers	
genonlydeltalist = []		#list of generator file time between packets
genonlybuffslist = []		#list of generator file intial buffer times
genonlycountlist = []		#list of generator file packet counts
genonlypsizelist = []		#list of generator file packet sizes
gen_rec_conflist = []		#list of stream ids that have generator and receiver files
#Figure out which streams have both receiver files and generator files
#For those that have no receiver, log the stream, hostname, receiver, time between packets, initial buffer, number of packets, and packetsize
while(r != len(generatorstrmlist)):
	if(len(receiverstrmlist) == 0):
		print "Generator file confirmation for stream id: ", generatorstrmlist[r][2], "**NO RECEIVER FILE**"
		genonlystrmlist.append(generatorstrmlist[r][2])
		genonlyhostlist.append(generatorstrmlist[r][7])
		genonlyreceiverlist.append(generatorstrmlist[r][8])
		genonlydeltalist.append(generatorstrmlist[r][4])
		genonlybuffslist.append(generatorstrmlist[r][5])
		genonlycount = generatorstrmlist[r][3]
		egenonlycount = genonlycount.encode("utf-8")
		lgenonlycount = long(egenonlycount)
		genonlycountlist.append(lgenonlycount)
		genonlypsizelist.append(generatorstrmlist[r][6])
		r = r + 1
		if(r == len(generatorstrmlist)):
			break
	if(len(receiverstrmlist) != 0):
		if(generatorstrmlist[r][2] == receiverstrmlist[t][2]):
			genconf = [1, generatorstrmlist[r][2]]
			print "Generator and receiver file confirmation for stream id: ", generatorstrmlist[r][2]
			gen_rec_conflist.append(generatorstrmlist[r][2])
			r = r + 1
			t = 0
			if(r == len(generatorstrmlist)):
				break
		if(t == (len(receiverstrmlist) - 1)):
			print "Generator file confirmation for stream id: ", generatorstrmlist[r][2], "**NO RECEIVER FILE**"
			genonlystrmlist.append(generatorstrmlist[r][2])
			genonlyhostlist.append(generatorstrmlist[r][7])
			genonlyreceiverlist.append(generatorstrmlist[r][8])
			genonlydeltalist.append(generatorstrmlist[r][4])
			genonlybuffslist.append(generatorstrmlist[r][5])
			genonlycount = generatorstrmlist[r][3]
			egenonlycount = genonlycount.encode("utf-8")
			lgenonlycount = long(egenonlycount)
			genonlycountlist.append(lgenonlycount)
			genonlypsizelist.append(generatorstrmlist[r][6])
			r = r + 1
			t = 0
			if(r == len(generatorstrmlist)):
				break
		if(generatorstrmlist[r][2] != receiverstrmlist[t][2]):
			t = t + 1	
num = 0
#This adds the number of packets in each stream together consecutively, needed for MEAN operation	
for node in istrmcountlist:
	if(len(istrmcountlist) == 1):
		fullcountlist.append(streamcountnum)
		break
	fullcountlist.append(streamcountnum)
	streamcountnum = fullcountlist[num] + istrmcountlist[num + 1]
	num = num + 1
	if(num == len(istrmcountlist) - 1):
		fullcountlist.append(streamcountnum)
		break

halfullcountlist = []	
i = 0
#This gets a list of numbers used for the indexes of the middle of the lists, needed for median
for node in istrmcountlist:
	halfullcountlist.append(node/2)

#Gather the packet tag information into a list
for f in filelist:
	index = 0
	#gather the data from the packets section of the .log file
	doc = xml.dom.minidom.parse(f)
	for node in doc.getElementsByTagName("packet") :	
		packets = [node.getAttribute("streamid"), node.getAttribute("seqnum"), node.getAttribute("status"), 
		node.getAttribute("date"), node.getAttribute("time")]
		packetlist.append(packets)
	#sort by stream id
	packetlist.sort(key = lambda packetlist:packetlist[0])

#Count information: get all packets that are ontime, missed or late individual streams
totalpackets = 0
totalpacks = [] #total number of packets being operated on
while (index != receiverfiles):
	for node in packetlist:
		#packets are ontime, late or missed
		if(node[0] == receiverstrmlist[index][2]):
			if(node[2] == "ontime"):
				ontime = ontime + 1
				totalpackets = totalpackets + 1
			if(node[2] == "late"):
				late = late + 1
				totalpackets = totalpackets + 1
			if(node[2] == "missed"):
				missed = missed + 1
				totalpackets = totalpackets + 1
	count = count - 1
	index = streamnum - count
	n = n + 1
	#Move all ontime, late, and missed packet information into lists
	cnt_ontimelist.append(ontime)
	cnt_latelist.append(late)
	cnt_misslist.append(missed)
	totalpacks.append(totalpackets)
	#reset values
	ontime = 0
	late = 0
	missed = 0
	totalpackets = 0
#Deals with duplicate packets
j = 0
for node in totalpacks:
	duplicate = node - strm_countlist[j]
	j = j + 1
	cnt_duplist.append(duplicate)

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
#timelist = a list of stream ids, packet numbers, and total microseconds
timelist.append(streamlist)
timelist.append(packetnumlist)
timelist.append(microseclist)

index = 0
place = 0	
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
#timinglist = stream ids and the interarrival time for the packets							
timinglist.append(timestreamlist)
timinglist.append(interarrivaltimelist)

#once the timinglist is created, separate the interarrival times by stream id so that mean, standard devation and median can be calculated 		according to stream id
count = 0
interarrive = 0	
i = 0
#MEAN
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

count = 0
i = 0
tot = 0

#STDEV
while(count < len(timinglist[0])):
	diff = timinglist[1][count] - time_meanlist[i]
	squared = (diff*diff)
	tot = tot + squared
	count = count + 1
	if(count == fullcountlist[i]):
		div = (tot//(istrmcountlist[i]))
		time_stddev = math.sqrt(div)
		time_stddevlist.append(time_stddev)
		tot = 0
		i = i + 1
#Conditional to get around generator files
if(len(timinglist[0]) != 0):
	timinglist.sort(key = lambda timinglist:timinglist[0])

i = 0
count = 0
begin = 0
#MEDIAN, MIN, MAX
while(count < len(timinglist[0])):
	medianlist.append(interarrivaltimelist[count])
	count = count + 1
	if(count == fullcountlist[i]):
		medianlist.sort(key = lambda medianlist:medianlist)
		mmedianslist.append(medianlist[halfullcountlist[i]])
		minlist.append(medianlist[begin])
		maxlist.append(medianlist[len(medianlist) - 1])
		medianlist = []
		i = i + 1

#This gathers the data available in generator files that have no receiver file.
#NOTE: Timing data and packet data does not exist, is replaced by -1
x = x + 1
d = 0
for node in genonlystrmlist:
	id_srclist.append(genonlyhostlist[d])	
	id_destlist.append(genonlyreceiverlist[d])
	id_streamidlist.append(genonlystrmlist[d])
	cnt_ontimelist.append(-1)
	cnt_latelist.append(-1)
	cnt_misslist.append(-1)
	cnt_duplist.append(-1)
	strm_buffslist.append(genonlybuffslist[d])
	strm_countlist.append(genonlycountlist[d] + 1)
	strm_deltalist.append(genonlydeltalist[d])
	strm_psizelist.append(genonlypsizelist[d])
	time_meanlist.append(-1)
	time_stddevlist.append(-1)
	mmedianslist.append(-1)
	minlist.append(-1)
	maxlist.append(-1)
	d = d + 1	

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

x = 0
#Takes the data from big list and puts it into the rowlist(each stream's data will finally then be all together in a list with the same index)		
#The rowlist is written into a .csv file using dictwriter
#This is repeated for the number of streams minus the length of the number of streams that have both a generator file and a receiver file 
while(x < (streamnum - len(gen_rec_conflist))):		
	for node in biglist:	
		rowlist.append(node[x])
	rowdict = {"id_src": rowlist[0], "id_dest": rowlist[1], "id_streamid": rowlist[2], "cnt_ontime(packets)": rowlist[3], 
	"cnt_late(packets)": rowlist[4], "cnt_miss(packets)": rowlist[5], "cnt_dup(packets)": rowlist[6], "strm_buffs(packets)": rowlist[7],
	 "strm_count(packets)": rowlist[8], "strm_delta(millisecs)": rowlist[9], "strm_psize(bytes)": rowlist[10], "time_mean(microsec)": 
	rowlist[11], "time_stddev(microsec)": rowlist[12], "time_med(microsec)": rowlist[13], "time_min(microsec)": rowlist[14], 
	"time_max(microsec)": rowlist[15]}
	dictwriter.writerow(rowdict)
	rowlist = []
	x = x + 1
		
fopen.close()
