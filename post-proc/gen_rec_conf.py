#!/usr/local/bin/python
#This is a class that will check to see if the generator logs and receiver logs agree.
#This is used to see whether or not an entire stream has been dropped or not

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
#fieldnames = ["id_src", "id_dest", "id_streamid", "cnt_ontime", "cnt_late", "cnt_miss", "cnt_dup", "time_mean", 
#"time_med", "time_stddev", "time_max", "time_min", "strm_delta", "strm_buffs", "strm_count", "strm_psize", "gen_conf", "rec_conf"]
#dictwriter = csv.DictWriter(fopen, fieldnames = fieldnames)
#dictreader = csv.DictReader(open(csvlog), fieldnames = "id_streamid")

filelist = set()
for i in range(1,len(sys.argv)):
	files = glob.glob(sys.argv[i])
	for f in files: 
		filelist.add(f)
print filelist
csvlog = "rtgenstats.csv"
fopen = open(csvlog)
reader = csv.reader(fopen)
genstreamlist = []
streamidlist = []
gen_conflist = []
rec_conflist = []
n = 0
i = 0
gen = 0
rec = 0
for f in filelist: 
	doc = xml.dom.minidom.parse(f)
	for node in doc.getElementsByTagName("streamstart") :
		streamid = node.getAttribute("streamid")
		genstreamlist.append(streamid)
genstreamlist.sort(key = lambda genstreamlist:genstreamlist)
for row in reader:
	streamidlist.append(row[2])
streamidlist = streamidlist[1:len(streamidlist)]
print genstreamlist
print streamidlist
print "number of generator files: ", len(genstreamlist)
print "number of receiver files: ", len(streamidlist)
while(i != len(genstreamlist)):
	if(genstreamlist[i] == streamidlist[n]):
		strmid = genstreamlist[i]
		print "receiver and generator files confirmed for streamid: ",strmid
		n = 0
		i = i + 1
		gen_conflist.append(1)
		rec_conflist.append(1)	
	n = n + 1
	if(n >= len(streamidlist)):
		strmid = genstreamlist[i]
		n = 0
		i = i + 1
		gen_conflist.append(0)
		print "generator file confirmed for streamid: ",strmid, "**no receiver confirmation**"	
fopen.close()		
