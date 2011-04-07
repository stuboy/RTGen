#include "streamInfo.h"
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <fstream>
#include <netdb.h>
#include <list>
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ostream>
#include <string.h>

using namespace std ; 

//Declaration of variables
list<StreamInfo*> streamList ; 
bool done ; 
pthread_t maintThread ; 
pthread_mutex_t maintThreadMutex ; 

StreamInfo * GetStreamInfo ( Packet * );
void * MaintThread ( void * );
StreamInfo * CreateNewStreamInfo ( Packet *, Logger * , char isRT, string IP) ;
void * EntryPoint ( void * ) ; 
bool IsNumber ( char * str ) ; 
string sender;
string portofcall = "9999";

int main ( int argc, char *argv[] ) {
	
	// initialize log file
	// initialize list of IDs
	// start ID list maintenance thread 
	// initialize socket 
	string address = "0.0.0.0" ; 
	string port = "9876" ; 
	string logfile = "" ; 
	done = false ; 
	bool error = false; 
	char state = ' ' ; 
	string temp ;
	string filename = "";
	Logger * logger ; 
	
	//Check for config file
	for (int f = 1; f < argc && !error ; f ++ ) {
		switch ( state ) {
			/**Added by Jared 6/19/08. To receive a config file for receiver.
			 **/
			case 'f' :  //Config File Assignment
				filename.assign(argv[f]) ;
				state = ' ';
				break;
			//edit end
			case ' ' : // Junk Remover/Invalid Parameter Catcher
				if ( argv[f][0] == '-' ) {
					state = argv[f][1] ; 
				}
				else {
					error = true ; 
				} 
				break ;  
			default :  break ; 
		}
	}
	//Edit by Jared on 6/19/08. Added ability to read config file denoted by -f option.
	if( filename != "" ){
		//open config file
		ifstream cfg(filename.c_str());
		//only if valid config file continue
		if(cfg.good()){
			char firstChar = ' ';
			char buff[1024];
			char tag[24];
			char value[24];
			//Read Config file and pull out assignments
			while( !cfg.eof() ){
				do{
				cfg.getline(buff,1024);
				firstChar = buff[0];
				}while((firstChar == '#') || (firstChar == '\n') || (firstChar == ' '));
				if((firstChar != '#') && (firstChar != '\n') && (firstChar != ' ')){\
					cfg.getline(buff, 1024);
					sscanf(buff, "%s %*s %s", tag, value);
				}
				//Tiny Lil Switch
				string compStr = tag;		
				if( compStr.compare("Port") == 0 ){
					if ( IsNumber ( value ) ) {
						port.assign ( value ) ;
					}else{
						error = true ; 
					}
				}else if ( compStr.compare("log") == 0 ){				
					logfile.assign(value) ;
				}else if ( compStr.compare("BackPort") == 0){
					if( IsNumber (value) ) {
						portofcall.assign( value );
					}else{
						error = true;
					}
				}
			} 
		}else{
			error = true;
		}
	}
	//end edit
	state = ' ';
	//Start handling of Command line parameters
	for (int c = 1; c < argc && !error ; c ++ ) {
		switch ( state ) {
			/**Added by Jared 6/19/08. To receive a config file for receiver.
			 **/
			case 'f' :  //Config File Assignment
				filename.assign(argv[c]) ;
				state = ' ';
				break;
			//edit end
			case 'p' : // port assignment
				if ( IsNumber ( argv[c] ) ) {
					port.assign ( argv[c] ) ;
				}
				else error = true ; 
				state = ' ' ;  
				break ; 
			case 'b' : // feedback port assignment
				if ( IsNumber ( argv[c] ) ) {
					portofcall.assign ( argv[c] ) ;
				}
				else error = true ; 
				state = ' ' ;  
				break ;
			case 'l' : // log file name 
				logfile.assign(argv[c]) ;
				state = ' ' ; 
				break;  
			case ' ' : // Junk Remover/Invalid Parameter Catcher
				if ( argv[c][0] == '-' ) {
					state = argv[c][1] ; 
				}
				else {
					error = true ; 
				} 
				break ;  
			default :  error = true ; break ; 
		}
	}
	//Error Message
	if ( error ) {
		cout << " Stu\'s Real-Time Traffic Receiver " << endl 
			<< "Usage: receiver -f <config filename> -p <port#> -b <feedback port#> -l <logfile_name> " <<endl
			<< "Explanation: " << endl 
			<< "-f = configuration filename (default: none) " << endl
			<< "-p = port number (default: 9876) " << endl
			<< "-b = feedback port number (default: 9999) " << endl 
			<< "-l = logfile name (omitting this option logs to stdout) " << endl  
			<< "(option order not important)" << endl ; 
		exit(0) ; 
	}
	//Initialize Logger
	logger = new Logger ( logfile ) ;
	//Clean Stream List 
	streamList.clear(); 
	//Initialize Socket
	int recvSocket = socket(AF_INET, SOCK_DGRAM, 0) ; 
	addrinfo *ai; 
	// name lookup
	struct addrinfo hints ; 
	memset(&hints, 0, sizeof(addrinfo) ) ; 
	hints.ai_family = AF_INET ; 
	hints.ai_socktype = SOCK_DGRAM ; 
	int result ; 
	//Check Address
	if ( (result = getaddrinfo(address.c_str(), port.c_str(), &hints, &ai ))){
		cout << "Error: getaddrinfo: " << result << endl ; 
		exit (-1 ) ;
	}
	
	//Set up Socket
	if ( (result = bind (recvSocket, ai->ai_addr, ai->ai_addrlen )) ){
		cout << "Error: bind: " << result << endl ; 
		exit (-1) ; 
	}
	//Initialize Threads
	pthread_mutex_init ( &maintThreadMutex, NULL ) ; 
	pthread_create( &maintThread, NULL, MaintThread, NULL ) ; 
	
	//Variables to use in receiving packets
	int bufferLength = 1500, 
		packetLength = 0;  
	u_char buffer[bufferLength] ; 
	sockaddr_storage from ; 
	socklen_t fromLength = sizeof(from) ;
	Packet * newPacket ; 
	StreamInfo * sInfo = NULL ; 
	//u_long node;
	char host[1025];
	char service[32];
	//Receive Loop
	while ( true ) {
		//Collect a Packet
		if ( ! (packetLength = recvfrom ( recvSocket, &buffer, bufferLength, 0, 
			(sockaddr*)&from, &fromLength )) ) {
			cout << "Error: recvfrom: " << packetLength << endl ;  
			exit(-1) ; 
		}
		//Check if All of Packet has Arrived
		if ( packetLength < sizeof(newPacket) ) {
			cout << "Warning: received packet too small: " << packetLength 
			<< " (sizeof: " << sizeof(newPacket) << ")" << endl ; 
			continue ; 
		}
		int wtf = 0;
		if((wtf = (getnameinfo((sockaddr*)&from,fromLength, host, sizeof(host), service,
				 sizeof(service), NI_NUMERICHOST|NI_NUMERICSERV|NI_DGRAM)))!=0){
			cout << "ERROR: GETNAMEINFO: " << wtf << endl;
		}
		sender.assign(host);
		//Put received packet back into Packet structure
		newPacket = new Packet () ; 
		memcpy(newPacket, &buffer, packetLength ) ; 
		/**Added by Jared on 6/26/08. Switched values from Big Endian back to Little Endian.
	 	* To make things more standard.
		 **/
		newPacket->version = ntohs(newPacket->version);
		newPacket->payloadLength = ntohs(newPacket->payloadLength);
		newPacket->streamId = ntohl(newPacket->streamId);
		newPacket->sequenceNumber = ntohl(newPacket->sequenceNumber);
		newPacket->finalSequenceNumber = ntohl(newPacket->finalSequenceNumber);
		newPacket->interDepartureTime = ntohs(newPacket->interDepartureTime);
		newPacket->initBufferCount = ntohs(newPacket->initBufferCount);
		//edits end
		sInfo = GetStreamInfo(newPacket) ; 
		/**Edits by Jared on 18th June 2008 to have receiver known what type of stream it is receiving
		 **/
		char isRT = newPacket->isRT;
		//edits end
		if ( sInfo == NULL ) {    //If no stream exists yet for these packets make one
			sInfo = CreateNewStreamInfo ( newPacket, logger, isRT, sender) ; 
			pthread_mutex_lock(&maintThreadMutex ) ; 
			streamList.push_front ( sInfo ) ; 
			pthread_mutex_unlock(&maintThreadMutex) ;  
		}
		else {    //The stream of this packet exists so add this packet to that stream
			//This is where an individual packet is added to a stream (Packet Received)
			sInfo->QueuePacket ( newPacket ) ;
			//If packet is part of a Non-Real Time Stream
			//Send Signal to sInfo thread that a packet has arrived
			if( newPacket->isRT == '0' ){
				sInfo->packet_signal();
			}
		}
	}
	
	return 0 ; 
}

/**Check if a string is comprised of only digits (0-9)
 * If so it can be translated into a number
 **/
bool IsNumber ( char * str ) {
	for ( int c = 0 ; str[c] != '\0' ; c ++ ) {
		if (str[c] > '9' || str[c] < '0' ) 
			return false ; 
	}
	return true ; 
}

/*
GetStreamInfo checks if the stream id in a packet is already part of a stream list or a new one. This is helpful in knowing what stream a packet belongs to when it arrives. If a packet belongs to an existing stream then the function returns a non-Null pointer else it returns a NULL pointer
*/
StreamInfo * GetStreamInfo ( Packet * packet ) {
	StreamInfo * retval  = NULL ;
	list<StreamInfo*>::iterator sInfo ; 
	bool done = false ; 
	pthread_mutex_lock(&maintThreadMutex) ; 
	for (sInfo = streamList.begin(); sInfo != streamList.end() && !done ; sInfo ++ ) 
	{
		if ((*sInfo)->GetStreamId() == packet->streamId ) {
			done = true ; 
			retval = (*sInfo) ; 
		} 
	}
	pthread_mutex_unlock(&maintThreadMutex ) ; 
	return retval ; 
}


StreamInfo * CreateNewStreamInfo ( Packet * packet, Logger * logger , char isRT, string ip) {
	StreamInfo * ret = new StreamInfo ( packet, logger, isRT, ip ) ; 
	pthread_create(&ret->thread, NULL, EntryPoint, (void*)ret ) ; 
	return ret ; 
}


void * EntryPoint ( void * param ) {
	StreamInfo * temp = (StreamInfo*)param ; 
	//This is where stream is actually processed.
	//Check if RT or NRT as they are processed differently
	if(temp->RT == '0' ){
		temp->NRTProcess();
	}else{
		temp->Process() ;
	} 
	temp->CleanUp() ; 
}

/*
This procedure executed by mainThread basically checks if all packets belonging to the various streams have been processed. The thread wakes up every 30 seconds to check. If a stream's packets are processed it erases that stream id from the stream list
*/
void * MaintThread ( void * ) {
	while ( !done ) {
		sleep(5) ;
		pthread_mutex_lock (&maintThreadMutex) ; 
		list<StreamInfo*>::iterator it ;
		list<StreamInfo*>::iterator delit ;
		for ( it = streamList.begin(); streamList.size() > 0 
			&& it != streamList.end() ; it ++ ) {
			if ( (*it)->Finished() ) {
				delit = it-- ; 
				streamList.erase(delit) ; 
			/**Added by Jared 6/18/08. A 16B Feedback UDP packet is sent back to the stream's sending receiver that
			 * tells the generator what stream it is how many packets are ontime, how many packets are late,
			 * and the current sequence number being processed.
			**/
			}else{  //Not Finished so send feedback to the generator responsible.
				//Variables to set up sending of feedback (socket and destination).				
				struct addrinfo * destInfo;
				struct addrinfo hints ; 
				memset(&hints, 0, sizeof(addrinfo) ) ; 
				hints.ai_family = AF_INET ; 
				hints.ai_socktype = SOCK_DGRAM ;
				//Create destination address info.
				int result;
				if ( (result = (getaddrinfo(((*it)->sender).c_str(), portofcall.c_str(), &hints, &destInfo ))) ){
					cout << "Error: getaddrinfo: " << result << endl ; 
					exit (-1 ) ;
				}
				//Create a socket to send from.
				int sock = socket ( destInfo->ai_family, destInfo->ai_socktype, 0 ) ; 
				if ( sock < 0 ) {
					cout << "Error: socket() " << sock << endl ;  
					exit (-1) ; 	 // error creating socket... bail. 
				}
				//Update status (id:ontimeCount:lateCount:curSequenceNumber).
				(*it)->GetStatus();
				//Assign four unsigned long values into 16B unsigned character array
				unsigned char info[sizeof((*it)->info)];
				memcpy(info,(*it)->info,sizeof((*it)->info));
				//Send to appropriate generator ip (Actual handling done by API.
				sendto(sock, info, sizeof(info),0,destInfo->ai_addr,destInfo->ai_addrlen);
			}//edits end
		}
		pthread_mutex_unlock(&maintThreadMutex) ; 
	}
}
