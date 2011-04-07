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
"time_med", "time_stddev", "time_max", "time_min", "strm_delta", "strm_buffs", "strm_count", "strm_psize", "gen_conf", "rec_conf"]
csvwriter = csv.writer(fopen)
csvwriter.writerow(fieldnames)
dictwriter = csv.DictWriter(fopen, fieldnames = fieldnames)
generatoridlist = []
for f in filelist: 
		#initialize variables
		streamnum = 0
		ontime = -1
		late = 0
		missed = 0
		duplicate = -1
		index = 0
		n = 0
		packnum = 0
		x = 0
		y = 0
		pack = 0
		lpacknum = 0
		lhours = 0
		lmins = 0
		lsecs = 0
		lmicrosecs = 0
		streamstartlist = []
		streamstoplist = []
		packetlist = []	
		id_srclist = []
		id_destlist = []
		id_streamidlist = []
		cnt_ontimelist = []
		cnt_latelist = []
		cnt_misslist = []
		cnt_duplist = []
		packetnumlist = []
		packettimelist = []
		strm_buffslist = []
		strm_countlist = []
		srtm_psizelist = []
		strm_deltalist = []
		strm_psizelist = []
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
		time_meanlist = []
		time_stddevlist = []
		medianlist = []
		biglist = []
		rowlist = []
		print "processing "
		doc = xml.dom.minidom.parse(f)
		for node in doc.getElementsByTagName("streamstart") : 
			streamstarts = [node.getAttribute("sender"), node. getAttribute("host"), node.getAttribute("streamid"), node.getAttribute("type"),
			node.getAttribute("iseqnum"), node.getAttribute("fseqnum"), node.getAttribute("proctime"), 
			node.getAttribute("initbuffs"), node.getAttribute("packetsize"), node.getAttribute("date"), node.getAttribute("receiver")]
			streamstartlist.append(streamstarts)
		streamstartlist.sort(key = lambda streamstartlist:streamstartlist[2])		
		#get info out of the streamstarts and put them into lists for further processing
		for node in streamstartlist:
			streamnum = streamnum + 1
			count = streamnum	
			id_src = node[0]
			id_dest = node[1]
			id_streamid = node[2]
			strm_buffs = node[7]
			strm_count = node[5]
			strm_delta = node[6]
			strm_psize = node[8]
			id_srclist.append(id_src)
			id_destlist.append(id_dest)
			id_streamidlist.append(id_streamid)
			strm_buffslist.append(strm_buffs)
			strm_countlist.append(strm_count)
			strm_deltalist.append(strm_delta)
			strm_psizelist.append(strm_psize)
			#used to get int indexes
			estrmcount = strm_count.encode("utf-8")
			istrmcount = int(estrmcount)
			istrmcountlist.append(istrmcount)
		#print istrmcountlist
		streamcountnum = istrmcountlist[0]#this line hangs up when trying to add generator logs**********
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
		halfullcountlist = []
		i = 0
		#print fullcountlist
		#This gets a list of numbers used for the indexs of the middle of the lists - helps get the median further on
		for node in istrmcountlist:
			halfullcountlist.append(node/2)
		#gather the data from the packets section of the .log file
		for node in doc.getElementsByTagName("packet") :	
			packets = [node.getAttribute("streamid"), node.getAttribute("seqnum"), node.getAttribute("status"), node.getAttribute("date"), 				node.getAttribute("time")]
			packetlist.append(packets)
		packetlist.sort(key = lambda packetlist:packetlist[0])
		#print packetlist
		#first attempt at count information: get all packets that are ontime, missed or late for the entire file, not individual streams
		#starts at the first stream in the sorted list
		while (index != streamnum):
			for node in packetlist:
				#packets are ontime, late or missed
				if(node[0] == streamstartlist[index][2]):
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
		#print timelist
		index = 0
		place = 0	
		done = False
		#split the timelist into a list of stream ids and the interarrival time between packets	
		while(index != streamnum):
			#created for gen/rec file confirmation
			if(len(timelist[0]) == 0):
				break
			#if the end of the list is reached, end the while loop			
			if((len(timelist[1])-1) == place):
				#print place
				index = index + 1
				break					
			#checking that streamids are the same
			if(timelist[0][place] == id_streamidlist[index]):
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
		#once the timinglist is created, separate the interarrival times by stream id so that mean, standard devation and median can be 		calculated according to stream id
		count = 0
		interarrive = 0	
		i = 0
		#cycle through the entire list
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
		#print time_meanlist
		count = 0
		i = 0
		tot = 0
		#STDDEV
		#print fullcountlist
		#print fullcountlist[0]
		#print fullcountlist[1]
		while(count < len(timinglist[0])):
			#print timinglist[1][count]
			#print time_meanlist[i]
			diff = timinglist[1][count] - time_meanlist[i]
			squared = (diff*diff)
			tot = tot + squared
			#print count 
			count = count + 1
			#print fullcountlist[i]
			if(count == fullcountlist[i]):
				time_stddev = math.sqrt(tot)
				time_stddevlist.append(time_stddev)
				#print i
				i = i + 1
		if(len(timinglist[0]) != 0):
			timinglist.sort(key = lambda timinglist:timinglist[0])####problem line with the gen/rec file confirmation
		#print timinglist
		i = 0
		count = 0
		begin = 0
		mmedianslist = []
		minlist = []
		maxlist = []
		#MEDIAN, MIN, MAX
		while(count < len(timinglist[0])):
			medianlist.append(interarrivaltimelist[count])
			count = count + 1
			if(count == fullcountlist[i]):
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
########	For the generator, receiver confirmation
		x = 0
		while(x < streamnum):
			if(streamstartlist[x][10] == " "):
				break
			else:
				for node in doc.getElementsByTagName("sstop") :	
					streamid = node.getAttribute("streamid")
					generatoridlist.append(streamid)
				x = x + 1
		print generatoridlist
#		genidlist.sort(key = lambda gen_conflist:gen_conflist[0])
		
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
		
		print streamnum
		#Takes the data from big list and puts it into the rowlist(each stream's data will finally then be all together in a list)
		#The rowlist is written into a .csv file using dictwriter
		#This is repeated for the number of streams in the .log file
		#print biglist
		while(x < streamnum):
			for node in biglist:
				rowlist.append(node[x])
			rowdict = {"id_src": rowlist[0], "id_dest": rowlist[1], "id_streamid": rowlist[2], "cnt_ontime": rowlist[3], 
			"cnt_late": rowlist[4], "cnt_miss": rowlist[5], "cnt_dup": rowlist[6], "strm_buffs": rowlist[7], "strm_count": rowlist[8], 				"strm_delta": rowlist[9], "strm_psize": rowlist[10], "time_mean": rowlist[11], "time_stddev": rowlist[12], "time_med": rowlist[13], 				"time_min": rowlist[14], "time_max": rowlist[15]}
			dictwriter.writerow(rowdict)
			rowlist = []
			x = x + 1
		
fopen.close()

