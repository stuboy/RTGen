/** 
\file 
**/ 

#include "packet.h"
#include "streamInfo.h"

#ifndef PACKETINFO_H
#define PACKETINFO_H

/** 
\class 
**/
class PacketInfo { 
public:
	long streamId ; 
	long sequenceNumber ; 
	timeval arrivalTime ; 
	bool onTime ; 
	bool isNRT;
	int timeDiff ; 	// in usec 
	void PrintInfo() ; 
	void UpdatePacketInfo ( Packet* ) ; 
	PacketInfo ( ) ; 
	Packet * packet ; 
};

#endif
