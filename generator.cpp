//included code to generate packets of varying size. Default size is 24 bytes. - Vivek (2/28/08)

#include "streamInfo.h"
#include "packet.h" 
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <fstream>
#include <netdb.h> 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ostream>
#include <pthread.h>


using namespace std; 

bool IsNumber ( char * str ); 

int main ( int argc, char *argv[] ) 
{
	//Declaration of Variables
	string addr = "" ;
	string filename = "" ; 
	struct addrinfo * destInfo ; 
	string port = "9876" ; 
	int result = 0 ; 
	int packetLength;
	struct timeval time1; 
	u_long randStream;	
	gettimeofday(&time1,NULL);
	randStream=(time1.tv_usec)*1000000;
	string logfile = "" ;
//	/**Added by Pete on 6/14/10**/
	//edits end
	Packet packet ; 
	packet.version = 0 ; 
	//generate a random number from 0 to 10000 and add to time
	srand(time(NULL));
	randStream=randStream+(rand() % 1000 +1);
	packet.streamId = time(NULL) + randStream; 
	packet.sequenceNumber = 0 ; 
	packet.payloadLength = 0; 
	packet.finalSequenceNumber = 100 ; 
	packet.interDepartureTime = 10 ; 
	packet.initBufferCount = 10 ;
	//edited by Jared to flag type of packet
	//default packet is RT
	packet.isRT = '1';
	//edits end        
	//edited by vivek to include a payload of 1200 bytes 
	//edited by Jared to include a payload of 1500 bytes
	int headerLength = sizeof(packet) - 1500;
	//edits end
		 
	bool error = false; 
	char state = ' ' ;
	//Check for config file 
	for (int f = 1; f < argc && !error ; f ++ ) {
		switch ( state ) {
			case 'f' : //Config Filename
				 filename.assign ( argv[f] );
				 state = ' ' ;
				 break ;
			case ' ' : // Removes '-' characters and catches incorrect characters
				if ( argv[f][0] == '-' ) {
					state = argv[f][1] ; 
				}
				else {
					error = true ; 
				} 
				break ;  
			default : state = ' '; break ; 
		}
	}
	//Edit by Jared on 6/19/08. Fixed 6/23/08
	//Added ability to read config file denoted by -f option.
	if( filename != "" ){
		ifstream cfg(filename.c_str());
		if(cfg.good()){
			char firstChar = ' ';
			char buff[1024];
			char tag[24];
			char value[24];
			while( !cfg.eof() ){
				do{
					cfg.getline(buff,1024);
					firstChar = buff[0];
				}while((firstChar == ' ') || (firstChar == '\n') || (firstChar == '#'));
					if((firstChar != ' ') && (firstChar != '\n') && (firstChar != '#') ){\
						sscanf(buff, "%s %*s %s", tag, value);
						//Big Ol Switch
						string compStr = tag;
					if(compStr.compare("dest_addr") == 0){
							addr.assign ( value ) ; 
					}else if(compStr.compare("dest_port") == 0){
						if ( IsNumber ( value ) ) {						
							port.assign ( value ) ;
						}else error = true ; 
					}else if(compStr.compare("init_buff") == 0){
						if ( IsNumber ( value ) ) {
							packet.initBufferCount = short(atoi(value)) ; 
						}
						else error = true ;
					}else if(compStr.compare("Payload") == 0){
						if ( IsNumber ( value )) {
							packet.payloadLength = short(atoi(value)) -headerLength; 
							//edited by Jared on 7/2/08. Max size of packet is 1524 B with 24B header.
							//Thus payload should not exceed 1500 B (1524 -24)
							if ( packet.payloadLength > (sizeof(Packet) - headerLength) ) {
								cout << "packet length must be between 24 and 1524 " << endl ; 
								error = true ;
							} 
						}
						else error = true ; 
					}else if(compStr.compare("depart_time") == 0) {
						if ( IsNumber ( value ) ) {						
							packet.interDepartureTime = short ( atoi (value) ) ;
						}
						else error = true ; 
					}else if(compStr.compare("num") == 0) {
						if ( IsNumber ( value ) ) {
							packet.finalSequenceNumber = short ( atoi(value) ) ;
						}
						else error = true ; 
					/**Added by Jared on 18th June 2008 to differentiate whether stream is RT or NRT
			 		**/
					}else if(compStr.compare("is_RT") == 0){
						string temp = value;
						if ( temp.compare("false") == 0 ){
							packet.isRT = '0';
						}else if ( temp.compare("true") == 0 ){
							packet.isRT = '1';	
						}else{
							error = true;
						}
					//edits end 
//					/**Added by Pete on 11th June 2010 to add a log file 
//					**/
					}else if(compStr.compare("log") == 0) {
						logfile.assign(value);
					}
				}
			}
			cfg.close();	
		}else{  //Filename is not a valid one
			error = true;
		}
	}
	//Config File Read Done
	state = ' ';
	for (int c = 1; c < argc && !error ; c ++ ) {
		switch ( state ) {
			case 'a' : // Destination Address 
				addr.assign ( argv[c] ) ; 
				state = ' ' ; 
				break ;
			case 'f' : //Config Filename
				 filename.assign ( argv[c] );
				 state = ' ' ;
				 break ;
			case 'p' : // Destination Port
				if ( IsNumber ( argv[c] ) ) {
					port.assign ( argv[c] ) ;
				}
				else error = true ; 
				state = ' ' ;  
				break ;
			case 'b' : // Buffer Size Allocation
				if ( IsNumber ( argv[c] ) ) {
					packet.initBufferCount = short(atoi(argv[c])) ; 
				}
				else error = true ; 
				state = ' ' ; 
				break ;
			case 's' : // Payload Size (24 -1524 bytes)
				if ( IsNumber ( argv[c] ) ) {
					packet.payloadLength = int(atoi(argv[c])) -headerLength; 
					/**edited by Jared on 7/2/08. Max size of packet is 1524 B with 24B header.
					 * Thus payload should not exceed 1500 B (1524 -24)
					**/			
					if ( packet.payloadLength > (sizeof(Packet) - headerLength) ) {
						cout << "packet length must be between 24 and 1524 " << endl ; 
						error = true ;
					} 
				}
				else error = true ; 
				state = ' ' ; 
				break ; 
			case 't' : // Interdepature Time (ms)
				if ( IsNumber ( argv[c] ) ) {
					packet.interDepartureTime = short ( atoi (argv[c]) ) ;
				}
				else error = true ; 
				state = ' ' ;  				
				break ; 
			case 'n' : // Number of Packets to Send
				if ( IsNumber ( argv[c] ) ) {
					packet.finalSequenceNumber = short ( atoi(argv[c]) ) ;
				}
				else error = true ; 
				state = ' ' ;
				break ; 
			/**Added by Jared on 18th June 2008 to differentiate whether stream is RT or NRT
			 **/
			case 'r' : // Whether packets are RT (0) or NRT (1)
				if ( argv[c][0] == '0' ){
					packet.isRT = '0';
				}else if (argv[c][0] == '1' ){
					packet.isRT = '1';	
				}else{
					error = true;
				}
				state = ' ';
				break;
			//edits end
//			/**Added by Peter on 11th June 2010 to add a log file
//			 **/
			case 'l' : // Log file name
				logfile.assign(argv[c]) ;
				state = ' ' ; 
				break; 				
			//edits end
			case ' ' : // Removes '-' characters and catches incorrect parameters
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
	//Command Line Read Done
//	/**Added by Pete on 6/14/10**/
	//edits end
	//edits by Vivek start. Assigning 'a' to the packet payload.
	if (packet.payloadLength > 0) {
	//	packet.payLoad=(u_char*)calloc(packet.payloadLength,sizeof(u_char));
		for (int i=0;i<packet.payloadLength;i++)
		{
			packet.payLoad[i]='a';
		}
	}
	packetLength = headerLength + packet.payloadLength ; 
	
	//edits by Vivek end
	//No IP address was specified
	if (addr =="" ) error = true; 

	//Error Handling
	if ( error ) {
		cout << " Stu\'s Real-Time Traffic Generator " << endl 
			<< "Usage: generator -a <receiver_address> -f <config filename> -p <port#> " 
			<< "-b <initial_buffer_size_in_bytes> -s <payload_size_in_bytes> "
			<< "-t <delay_in_ms> -n <number_of_packets> -r <RT = 1/NRT = 0> -l <log_file_name>" << endl
			<< "Explanation: " << endl 
			<< "-a = receiver\'s address or name (no default)" << endl 
			<< "-f = config filename (no default)" <<endl
			<< "-p = port number (default: 9876) " << endl 
			<< "-b = number of packets to buffer before starting (default = 10) " << endl
			<< "-s = Payload size (24-1524) (default = 160 B)" << endl 
			<< "-t = delay between packets, in ms (default=20ms)" << endl 
			<< "-n = number of packets to send (default=100)" << endl 
			<< "-r = '1' for RT packets '0' for NRT packets (default= 1)" << endl
			<< "-l = name of the log file (default=generator.log)" << endl
			<< "(option order not important)" << endl ; 
		exit(0) ; 
	}
	
	//Print Stream Info to console
	cout << "Stream to "<< addr << ":" << port 
		<< " n=" << packet.finalSequenceNumber << "packets" 
		<< "  s=" << packet.payloadLength + headerLength << "Bytes" 
		<< "  d=" << packet.interDepartureTime << "ms" 
		<< " buffer=" << packet.initBufferCount << " "
		<< " RT=" << packet.isRT << "(1=RT/0=NRT)" 
		<< " log file=" << logfile << endl;
		
	// name lookup
	struct addrinfo hints ; 
	memset(&hints, 0, sizeof(addrinfo) ) ; 
	hints.ai_family = PF_INET ; 
	hints.ai_socktype = SOCK_DGRAM ; 
	if ( (result = (getaddrinfo(addr.c_str(), port.c_str(), &hints, &destInfo ))) ){
		cout << "Error: getaddrinfo: " << result << endl ; 
		exit (-1 ) ;
	}
	
	// create socket
	int sock = socket ( destInfo->ai_family, destInfo->ai_socktype, 0 ) ; 
	if ( sock < 0 ) {
		cout << "Error: socket() " << sock << endl ;  
		exit (-1) ; 	 // error creating socket... bail. 
	}
	
	// create packet data array
	u_char bytePacket[packetLength]; 
	// clear it
	memset(&bytePacket, 0, packetLength); 
	/**Added by Jared on 6/26/08. Switched values from Little endian to Big Endian.
	 * To make things more standard.
	 **/
	//Hold onto finalSequenceNumber to use it for upcoming for loop.
	u_long finalNum = packet.finalSequenceNumber;
	//Hold onto delay to use for sleep function in for loop.
	short delay   = packet.interDepartureTime;
	packet.version = htons(packet.version);
	packet.payloadLength = htons(packet.payloadLength);
	packet.streamId = htonl(packet.streamId);
	packet.finalSequenceNumber = htonl(packet.finalSequenceNumber);
	packet.interDepartureTime = htons(packet.interDepartureTime);
	packet.initBufferCount = htons(packet.initBufferCount);
	//edits end
	// copy the packet struct into it
	memcpy ( &bytePacket, &packet, packetLength ) ; 
//	/**Added by Pete on 6/17/10**/
	stringstream ss;
	ofstream gLog;
	gLog.clear();
	gLog.open(logfile.data()) ;
	//edits end
	/**Added by Pete on 6/21/10**/
	//microsecond timing
	char buffer[30]; 		
	struct timeval tv;
	time_t curtime;
	gettimeofday(&tv, NULL); 
 	curtime=tv.tv_sec;
 	strftime(buffer,30,"%m-%d-%Y  %T.",localtime(&curtime));
	//edits end
//	/**Added by Pete on 6/14/10**/	
	char hostname[20] ; 
	gethostname(hostname, 20) ; 
	//edits end
	//Send packets from 0 to N to destination
	for ( packet.sequenceNumber = 0 ; packet.sequenceNumber < finalNum ; 
		packet.sequenceNumber++ ) {

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
		Date and time
		char strTime[20]; 
		time_t time1 = time(NULL) ; 
		tm * time2 = localtime ( &time1 ) ; 
		strftime( &strTime[0], 20, "%Y-%m-%dT%H:%M:%S", time2 ) ; **/

		//Streamstart
		if(packet.sequenceNumber == 0) {
			ss << "<?xml version=\"1.0\" encoding=\"ascii\"?> " << endl ; 
			ss << "<log node=\"" << hostname << "\" date=\"" 
			<< buffer << tv.tv_usec << "\">" << endl ; 
			gLog << ss.rdbuf();
			ss << "<streamstart " << "streamid=\"" << htonl(packet.streamId) << "\"";
			gLog << ss.rdbuf();
			if(packet.isRT == '0') {
			ss << " type=\"NRT\"";
			gLog << ss.rdbuf();
			}else{
			ss << " type=\"RT\"";//edits end
			gLog << ss.rdbuf();
			}
			/**Changed to receiver on 8/9/2010, now correct**/
			ss << " receiver=\"" << addr << "\""
			<< " iseqnum=\"" << packet.sequenceNumber << "\""
			<< " fseqnum=\"" << finalNum - 1 << "\""
			<< " proctime=\"" << delay << "\""
			<< " initbuffs=\"" << htons(packet.initBufferCount) << "\""	
			/**Added time= by Pete on 7/13/2010**/
			<< " packetsize=\"" << htons(packet.payloadLength) + 24 << "\""
			<< " hostname=\"" << hostname << "\""
			<< " date=\"" << buffer << tv.tv_usec << "\"/>\n";
			gLog << ss.rdbuf();
		}
		//edits end
		//Flip sequence number for network semantics
		packet.sequenceNumber = htonl(packet.sequenceNumber);		
		memcpy ( &bytePacket, &packet, packetLength ) ; 
		sendto(sock, &bytePacket, packetLength, 0, destInfo->ai_addr, destInfo->ai_addrlen ) ;
		//Flip sequence number back to host semantics 
		packet.sequenceNumber = ntohl(packet.sequenceNumber);
		//This adds the delay between the packets. Default delay is 20 ms (20 us * 1000)
		usleep ( (delay) * 1000);
//		/**Added by Pete on 6/14/10**/
		//Streamstop
		if(packet.sequenceNumber == (finalNum - 1)) { 
			ss << "<streamstop " << "streamid=\"" << htonl(packet.streamId) << "\""
			<< " date=\"" << buffer << tv.tv_usec << "\"/>\n";
			gLog << ss.rdbuf();
		}	
	}
	return 0 ; 
	gLog.close();
	//edits end
}

/**Checks if input string is a composed of all digits (0-9)
 * and thus can be translated to a number.
 **/
bool IsNumber ( char * str ) {
	for ( int c = 0 ; str[c] != '\0' ; c ++ ) {
		if (str[c] > '9' || str[c] < '0' ) 
			return false ; 
	}
	return true ; 
}
