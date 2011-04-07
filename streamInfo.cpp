/** 
\file
**/ 

#include <sys/time.h>
#include <sys/errno.h>
#include <netdb.h>
#include "streamInfo.h"
#include <string.h>

//#include </usr/include/glib-2.0/glib.h>

using namespace std ; 
#define BUF_LENGTH 20

/**Print Info to Console**/
void StreamInfo::PrintInfo( ) {
	cout << "SInfo: stream=" << streamId % 100000
		<< " procTime=" << processingTime 
		<< " finSeq#=" << finalSequenceNumber 
		<< " buf#= " << initBufferCount 
		<< " nextSeq#=" << nextSequenceNumber 
		<< endl ; 
}

/**StreamInfo Constructor
 * Initialize variables
 **/
StreamInfo::StreamInfo ( Packet * packet, Logger * inLogger, char isRT, string ip ) {
	// create queue
	// queue current packet
	// start thread 
	sender = ip;
	allDone = false ;
	RT = isRT;
	gettimeofday(&beginTime, NULL);
	packetList.clear() ; 
	streamId = packet->streamId; 
	packetSize = packet->payloadLength ;
	initSequenceNumber = packet->sequenceNumber ; 
	initBufferCount = packet->initBufferCount ; 
	processingTime = packet->interDepartureTime ; 
	nextSequenceNumber = initSequenceNumber ; 
	finalSequenceNumber = packet->finalSequenceNumber - 1; 
	logger = inLogger ; 
	LogStreamStart ( ip ) ; 
	ontimeCount = lateCount = dropCount = oooCount = 0 ; 
	pthread_mutex_init ( &mutex, NULL ) ; 
	pthread_cond_init (&packetReceive, NULL );
	QueuePacket ( packet ) ; 
}

/**Process used by streams that contain Non-Real Time packets
 * Instead of polling, a signal is sent when a packet arrives and it is handled
 * on arrival. Also removes need to do any intial buffering.
 **/
void StreamInfo::NRTProcess ( ) {
	//Variables
	double list_length;
	bool timeout = false;
	bool error = false;
	long secs = 30;
	struct timespec time;
	struct timeval tempTime;
	//Handle Packets Loop
	while((!timeout) && (!error)){
		//^Not done, haven't timed out, no errors
		//Check for pending packets/Update list length
		pthread_mutex_lock (&mutex);
		list_length = packetList.size();
		if((list_length == 0)) {   //No Packets are pending
			//Create timeout marker
			gettimeofday(&tempTime, NULL);
			time.tv_sec = tempTime.tv_sec;
			time.tv_nsec = tempTime.tv_usec *1000;
			//Add 30 secs to make timeout time 30 secs after this time
			time.tv_sec += secs;
			//Wait till packet recieved or timeout
			switch(pthread_cond_timedwait(&packetReceive, &mutex, &time)){
				case 0: //Signal Received
					//Make sure packet is really there				
					list_length = packetList.size();
					if(list_length > 0){
						nextSequenceNumber = process_nPacket(nextSequenceNumber);
					}  //If no packets do nothing
					pthread_mutex_unlock(&mutex);
					break;
				case ETIMEDOUT:  //timeout or error
					//check if packet was received to be sure
					list_length = packetList.size();
					if(list_length == 0){ //no packets in list, we have timed out
						timeout = true;
					}else{                //Packet(s) in list, fake time out
						nextSequenceNumber = process_nPacket(nextSequenceNumber);
					}
					pthread_mutex_unlock(&mutex);
					break;
				default:  //error occured
					error = true;	
					pthread_mutex_unlock(&mutex);
					break;
			} //End Switch
		}else{              //Packet(s) are pending
			nextSequenceNumber = process_nPacket(nextSequenceNumber);
			pthread_mutex_unlock(&mutex);
		}                  //End packet pending case	
	}// End Handle Packet loop
	if(error){ //error handling
		cout << "Timedwait() error!" << endl;
	}
	//Log Statistics
	//LogStreamStats();
	LogStreamStop();
}

/**Process used by streams that contain Real Time packets
 * As packets arrive, the first few are stored until a predetermined amount (initBufferCount)
 * are received. Then the packet list is polled to see if any packets are there. If there is
 * some then handle them until the packet list is back to zero. Continue this process until
 * finished or timed out due to lack of incoming packets.
 **/
void StreamInfo::Process ( ) { 
	bool done  = false ;
	bool timeout = false; 
	double list_length;
	double old_length;
	double counter=0;
	double count=0;
	/*
	added by Vivek on 11/07/07
	Implement the buffer part where the thread sleeps until the buffer is full with 'x' packets or until the end of the transmission duration.
	*/
	pthread_mutex_lock ( &mutex );
	list_length=packetList.size();
	pthread_mutex_unlock ( &mutex ) ;
	/*
	edited by Jared on 6/18/08
	While loop now ignores const BUF_SIZE and instead uses variable initBufferCount
	*/
	//Initial Buffering 
	//After collecting initBufferCount packets
	while ((list_length < initBufferCount)&&(!timeout)) {
		old_length=list_length;
		usleep (processingTime * 1000 );
		//Check length of list
		pthread_mutex_lock ( &mutex );
		list_length=packetList.size();
		pthread_mutex_unlock ( &mutex ) ;
		//Compare with old length to see if packets have arrived
		if (old_length==list_length){	  //No new packets have arrived
			count++;                      //Increase non-acitivty count
		}
		else{                             //A packet has arrived
			old_length=list_length;
			count =0;                     //Reset non-activity count
		}
		if (count >3){                    //Non-activity count has exceded three
			counter=(finalSequenceNumber-nextSequenceNumber);
			//Timeout Timer started
			while((old_length==list_length)&&(counter!=0)){
				old_length=list_length;
				usleep (processingTime * 1000);
				//Check list in hopes new packet has arrived	
				pthread_mutex_lock ( &mutex );
				list_length=packetList.size();
				pthread_mutex_unlock ( &mutex ) ;
				counter--;
			}
			//Timeout has occured
			if(counter==0){
				timeout=true;
			}
		}	
	}//End initial buffering
	/**Edited by Jared 6/18/08
	 * removed rebuffing option, only completion or timeout exits loop
	 **/
	while ((!done) && (!timeout)){
		while((list_length > 0)&&(nextSequenceNumber!=finalSequenceNumber)){
			//process the packets
			pthread_mutex_lock ( &mutex ) ; 
			process_pkt(nextSequenceNumber);
			pthread_mutex_unlock ( &mutex ) ;
			//Increase sequence number
			nextSequenceNumber=nextSequenceNumber+1;
			//Take a nap
			usleep ( processingTime * 1000);
			//Recheck list length
			pthread_mutex_lock ( &mutex ) ; 
			list_length=packetList.size();
			pthread_mutex_unlock ( &mutex ) ;
		}
		//List is empty, but not done
		if(list_length==0)
		{
			//Timeout loop set-up
			//nextSequenceNumber=nextSequenceNumber+1;
			timeout=false;
			count=0;
			counter=0;
			
			while ((list_length == 0) && (!timeout)){
				usleep (processingTime * 1000);
				//Update list length
				pthread_mutex_lock ( &mutex );
				list_length=packetList.size();
				pthread_mutex_unlock ( &mutex ) ;
					
				if (list_length == 0)  //Still haven't received anything yet
					count++;
				else{                  //Received a packet
					count =0;
				}
				if (count >3){		   //Timeout
					counter=(finalSequenceNumber-nextSequenceNumber);
					while((list_length==0)&&(counter!=0)){  //Attempt a recovery 
						old_length=list_length;
						usleep (processingTime * 1000);
						pthread_mutex_lock ( &mutex );
						list_length=packetList.size();
						pthread_mutex_unlock ( &mutex ) ;
						counter--;
					}
					if(counter==0){   //timed out
						timeout=true;
					}
				}
			}
		}
		else{//processing of final packet
			pthread_mutex_lock ( &mutex ) ; 
			process_pkt(finalSequenceNumber);
			list_length=packetList.size();
			//LogStreamStats();
			LogStreamStop( );
			pthread_mutex_unlock ( &mutex ) ;
			done=true;
		}
	}
	if (timeout||(list_length>0)){	
		while(list_length!=0){
			pthread_mutex_lock ( &mutex ) ; 
			process_pkt(finalSequenceNumber);
			pthread_mutex_unlock ( &mutex ) ;
			pthread_mutex_lock ( &mutex ) ; 
			list_length=packetList.size();
			pthread_mutex_unlock ( &mutex ) ;
		}
		//LogStreamStats();
		LogStreamStop();
	}		
}

/**Processing of a RT packet
 * RT packets can be late so packet list must be searched until the current sequence 
 * number is found and anything with a number less than the current must be maked as
 * late and removed.
 **/
void StreamInfo::process_pkt (u_long currentSeqNum) {
	list<Packet*>::iterator it;
	bool found=false;
	//search for packet with currentSeqNum in list
	it = packetList.begin();
	while(!found && packetList.size() > 0 && it != packetList.end()){
		if ( (*it)->sequenceNumber < currentSeqNum) {
			LogPacket ( *it, LATE ) ; 
			lateCount ++ ; 
			it=packetList.erase(it);
		} else if ((*it)->sequenceNumber==currentSeqNum) {
			LogPacket( *it, ONTIME ) ; 
			ontimeCount ++ ; 
			it=packetList.erase(it); 
			found = true ;
		}
		else if(it != packetList.end())
			it++;
	}
}

/**Processing of a NRT packet
 * RT packets can't be late so the current packet is either marked as ontime or
 * out of order (ooo). 
 **/
u_long StreamInfo::process_nPacket (u_long currentSeqNumber) {
	list<Packet*>::iterator it;
	//If first packet is not next then it is out of order
	//Regardless as long as the packet has arrived it is on time.
	it = packetList.begin();
	/**Added by Jared 6/25/08. Might as well process all pending packets, enter while loop.
	 **/
	while((packetList.size() > 0) && (it != packetList.end())){ 
		if ( ((*it)->sequenceNumber < currentSeqNumber)) {
			LogPacket ( *it, ONTIME ) ;
			ontimeCount ++;
			oooCount ++ ; 
			it=packetList.erase(it);
		} else if ((*it)->sequenceNumber >= currentSeqNumber) {
			LogPacket( *it, ONTIME ) ; 
			ontimeCount ++ ; 
			currentSeqNumber = ((*it)->sequenceNumber + 1);
			it=packetList.erase(it); 
		}
	}
	return currentSeqNumber;
}
	
/**Get Stream's Id Number**/
u_long StreamInfo::GetStreamId ( ) {
	return streamId ; 
}

/**Signal that a NRT packet has arrived**/
/**Edit by Jared 18th June 2008 to apply to signalling performed by NRT process
 **/
void StreamInfo::packet_signal() {
	//get mutex
	pthread_mutex_lock (&mutex) ;
	//send signal
	pthread_cond_signal (&packetReceive);
	//release mutex
	pthread_mutex_unlock (&mutex) ;	
}
//end edit

/**Add a packet to the end of this stream**/ 
void StreamInfo::QueuePacket ( Packet * packet ) {
	// get mutex
	pthread_mutex_lock ( &mutex ) ;
	// push_back ( packet ) 
	packetList.push_back ( packet ) ; 
	// release mutex
	pthread_mutex_unlock ( &mutex ) ; 
}

/**Log a packet's arrival. A packet can arrive as either ontime, late, out of order,
 * or dropped entirely.
 **/
void StreamInfo::LogPacket ( Packet * packet, enum PacketAction action ) {
	string status ; 
	int clo = clock();
		switch (action ) {
			case ONTIME: status = "ontime" ; break ; 
			case LATE: status = "late" ; break ; 
			case DROP: status = "missed" ; break ; 
			default : break ; 
		}
	stringstream ss ; 
	
	//microsecond timing
	char buffer[30];
 	struct timeval tv;
	time_t curtime;
	gettimeofday(&tv, NULL); 
  	curtime=tv.tv_sec;
 	strftime(buffer,30,"%m-%d-%Y\" time=\"%T.",localtime(&curtime));

	//char strTime[20]; 
	//time_t time1 = time(NULL) ; 
	//tm * time2 = localtime ( &time1 ) ; 
	//strftime( strTime, 20, "%Y-%m-%dT%H:%M:%S", time2 ) ; 
	ss << "<packet streamid=\"" << packet->streamId << "\"" 
		<< " seqnum=\"" << packet->sequenceNumber << "\"" 
		<< " status=\"" << status << "\"" 
		<< " date=\"" <<  buffer << tv.tv_usec << "\"/>";
	logger->Log( ss.str() );   
}

/**Print Basic Stream Stats to the log file and mark start of a stream's processing**/
void StreamInfo::LogStreamStart ( string IP ) {
	/**Added by Pete on 6/14/10**/
	char hostname[20] ; 
	gethostname(hostname, 20) ; 
	//edits end
	stringstream ss ; 
	
	/**Added by Pete on 6/21/10**/
	//microsecond timing
	char buffer[30];
 	struct timeval tv;
	time_t curtime;
	gettimeofday(&tv, NULL); 
  	curtime=tv.tv_sec;	
	/**Added time= by Pete on 7/13/2010**/
 	strftime(buffer,30,"%m-%d-%Y\" time=\"%T.",localtime(&curtime));

	/** Old Date and Time
	char strTime[20]; 
	time_t time1 = time(NULL) ; 
	tm * time2 = localtime ( &time1 ) ; 
	strftime( &strTime[0], 20, "%Y-%m-%dT%H:%M:%S", time2 ) ; **/

	ss << "<streamstart streamid=\"" << streamId << "\"" ;
	if(RT == '0'){
		ss << " type=\"NRT\"";
	}else{
		ss << " type=\"RT\"";
	}
	ss << " sender=\"" << IP << "\""
		<< " iseqnum=\"" << initSequenceNumber << "\""
		<< " fseqnum=\"" << finalSequenceNumber << "\""
		<< " proctime=\"" << processingTime << "\"" 
		<< " initbuffs=\"" << initBufferCount << "\"" 
		/**Added packetsize by Pete on 7/13/2010**/
		<< " packetsize =\"" << packetSize + 24 <<"\""
		<< " host=\"" << hostname << "\""
		<< " date=\"" << buffer << tv.tv_usec << "\"/>";
		//edits end
	logger->Log( ss.str() );   
}

/**Print Stream Stop and Time to log file**/
void StreamInfo::LogStreamStop ( ) {
	stringstream ss ; 
	
	/**Added by Pete on 6/21/10**/
	//microsecond timing
	char buffer[30];
 	struct timeval tv;
	time_t curtime;
	gettimeofday(&tv, NULL); 
  	curtime=tv.tv_sec;
 	strftime(buffer,30,"%m-%d-%Y\" time=\"%T.",localtime(&curtime));

	//char strTime[20]; 
	//time_t time1 = time(NULL) ; 
	//tm * time2 = localtime ( &time1 ) ; 
	//strftime( &strTime[0], 20, "%Y-%m-%dT%H:%M:%S", time2 ) ; 

	ss << "<streamstop streamid=\"" << streamId << "\"" 
		<< " date=\"" << buffer << tv.tv_usec <<"\"/>" ;
		//edits end
	logger->Log( ss.str() );
}

/**After stream has been processed print the amount received, late, out of order,
 * and dropped to the logfile. 
 * If stream is NRT then print Packets/Sec and Thruput (B/S) to the logfile.
 **/
/**
void StreamInfo::LogStreamStats ( ) {
	stringstream ss ; 
	char strTime[20]; 
	//Added by Jared 6/20/08. Calculate total time spent on stream to determine
	//throughput for NRT streams
	long pps;
	long throughput;
	if(RT == '0'){      //Only need to do this for NRT streams
		struct timeval finTime;
		gettimeofday(&finTime,NULL);
		long elapsed_secs = finTime.tv_sec - beginTime.tv_sec;
		long elapsed_usecs = finTime.tv_usec - beginTime.tv_usec;
		//Return in ms
		//Adding 0.5  works as a rounding value
		long totalElapSecs = (elapsed_secs + (elapsed_usecs/1000000.0)) +0.5;
		long totalPackReceived = ontimeCount + oooCount;
		pps = (totalPackReceived / totalElapSecs);
		//As a note throughput is only calculated for how many bytes of data, NOT INCLUDING THE HEADER OF 24B
		//Thus a thruput of 7200 means 7200 bytes of data are processed a second.
		long totalBytesRecv = totalPackReceived * packetSize;
		throughput = (totalBytesRecv / totalElapSecs);
	}
	//edits end
	time_t time1 = time(NULL) ; 
	tm * time2 = localtime ( &time1 ) ; 
	strftime( &strTime[0], 20, "%Y-%m-%dT%H:%M:%S", time2 ) ; 
	//Added by Jared 7/1/08. Actually determine dropped count. 
	dropCount = (finalSequenceNumber + 1) - (ontimeCount + lateCount); //+ oooCount);
	//edits end
	ss << "<streamstats streamid=\"" << streamId << "\""  
		<< " ontime=\"" << ontimeCount << "\"" 
		<< " late=\"" << lateCount << "\""
		<< " dropped=\"" << dropCount << "\""  
		<< " ooo=\"" << oooCount << "\"" ; 
		//Added by Jared 6/20/08. Display Packets per second (PPS) and Throughput of NRT streams
		if ( RT == '0'){
			ss << " pps=\"" << pps << "\""
				<< " thruput=\"" << throughput << "\"" ;
		}
		//edits end	
		ss << " date=\"" << strTime << "\"/>" ;
	logger->Log( ss.str() );   
}
**/
/**Destroy the mutex and clear the packetList**/
void StreamInfo::CleanUp ( ) {
	pthread_mutex_destroy ( &mutex ) ;
	packetList.clear() ;  
	allDone = true ; 
}

/**Added by Jared 6/18/08. GetStatus updates a 16B unsigned character array with the current values of streamId,
 * ontimeCount,lateCount, and nextSequenceNumber which are then pulled out by receiver.cpp and sent back to the
 * sending generator for feedback.
**/
void StreamInfo::GetStatus(){
	//Create a temporary struct to hold the 16B of important data	
	struct mailman{
		u_long id;
		u_long punctual;
		u_long tardy;
		u_long curSeq;	
	}delivery;
	//Need to flip values for network traversal.
	delivery.id = htonl(delivery.id = streamId);
	delivery.punctual = htonl(delivery.punctual = ontimeCount);
	delivery.tardy = htonl(delivery.tardy = lateCount);
    	delivery.curSeq = htonl(delivery.curSeq = nextSequenceNumber);
	//Copy data in a 16B unsigned character array for easy transport and access from receiver
	memcpy(info,&delivery,sizeof(delivery));
}//edits end.

/**If true will tell Receiver that stream is finished
 * Otherwise nothing will occur
 **/
bool StreamInfo::Finished( ) {
	return allDone ; 
}


StreamInfo::~StreamInfo ( ) {
	delete (this ) ; 
}
