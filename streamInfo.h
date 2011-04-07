/** 
\file 
StreamInfo...
**/ 

#include "packet.h"
#include <iostream>
#include <list>
#include <fstream>
#include "logger.h"
#include <sstream>

using namespace std; 

#ifndef STREAMINFO_H
#define STREAMINFO_H

enum PacketAction {ONTIME, LATE, DROP, OOO}; 

class StreamInfo {
private: 
	struct timeval beginTime;
	list<Packet*> packetList ; 
	pthread_mutex_t mutex ; 
	u_long streamId ;
	u_long packetSize; 
	u_long initSequenceNumber ; 
	u_long nextSequenceNumber ; 
	u_short processingTime ;
	u_short playingTime; 
	u_long finalSequenceNumber ; 
	u_short initBufferCount ; 
	bool allDone ; 
	Logger * logger ; 
	u_long ontimeCount, lateCount, dropCount, oooCount ; 
	
	void LogPacket ( Packet *, enum PacketAction ) ; 
	void LogStreamStart ( string IP ) ; 
	void LogStreamStop () ; 
	void LogStreamStats ( ) ; 

public: 
	void PrintInfo( ) ; 
	StreamInfo ( Packet * packet, Logger * , char isNRT, string IP) ; 
	~StreamInfo( ) ;
	string sender;
	u_long GetStreamId ( ) ; 
	void QueuePacket ( Packet * packet ) ; 
	pthread_t thread ;
	pthread_cond_t packetReceive;
	char RT; 
	void NRTProcess();
	void Process () ; 
	void CleanUp() ; 
	bool Finished() ; 
	void process_pkt(u_long);
	u_long process_nPacket(u_long);
	void packet_signal();
	void GetStatus();
	unsigned char info[16];

};

#endif
