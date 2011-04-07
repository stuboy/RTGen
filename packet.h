/**
\file 
This file contains the struct for a packet
**/ 

#include <sys/types.h>

#ifndef PACKET_H
#define PACKET_H

struct Packet {
/** generator protocol version; 0 for now. 
**/
u_short version ; 

/** the length of the packet; though we should have 87 different ways to find 
	this, I threw this in anyway.
	**/ 
u_short payloadLength ; 

/** the ID of the stream; likely to be the time from time();
**/ 
u_long streamId; 

/** the sequence number of the packet; this starts at 0, but the first one
 	received by the receiver may be >0 due to packet loss. 
	**/ 
u_long sequenceNumber ; 

/** this is the final sequence number of the stream (known beforehand); 
	basically this tells you how long the stream is so you know when you have
	reached the end. 
	**/ 
u_long finalSequenceNumber ; 

/** the interdeparture time of the packets.  this is in every packet so when 
we miss the first packet, we won't be completely out of luck.  Specified in 
<i>ms</i>.

if this == 0xffff, this is not a real-time packet. 
**/
u_short interDepartureTime ; 

/** if this is >0, the first (specified) number of packets should be buffered
before we start looking for missed deadlines.  This will move our start time
back to the specified packet.  
\todo Decide how to handle missed packets with this value. 
**/ 
u_short initBufferCount ;

/**Added by Jared on 19th June 2008 to differentiate between RT and NRT packets.
 **/
u_char isRT;

/**edited by Vivek on 5th November 2007 to include a packet payload upto 1200 bytes long.
**/
/**edited by Jared on July 2 2007 to include a packet payload upto 1500 bytes long.
 * Gives packet ability to mimic video packets which can range from 65 to 1518 bytes.
**/
u_char payLoad[1500];  

};
#endif
