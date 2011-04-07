/** 
\file
**/ 

#include "packetInfo.h"
#include <iostream> 
#include <sys/time.h> 

using namespace std ; 

void PacketInfo::PrintInfo ( ) {
	cout<< "PInfo: stream=" << packet->streamId % 100000
		<< " seq#=" << packet->sequenceNumber % 100000
		<< " arr=" << arrivalTime.tv_sec % 10000 << "." << arrivalTime.tv_usec 
		<< " diff=" << timeDiff 
		<< " onTime?=" << onTime 
		<< endl ; 
}

void PacketInfo::UpdatePacketInfo ( Packet *inPacket ) {
	packet = inPacket ; 
	gettimeofday(&arrivalTime, NULL ) ; 
}

PacketInfo::PacketInfo ( ) {
}
