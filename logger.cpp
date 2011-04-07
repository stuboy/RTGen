
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

using namespace std;

Logger::Logger ( string filename ) {
	/**Edited by Jared on 6/27/08. This was incorrectly reading "" so changed to "\"\"" 
	 * to fix this issue.
	 **/
	if ( filename.compare("\"\"") != 0 ) {
		logToFile = true ; 
		logstream.open(filename.c_str()) ; 
		if ( logstream.bad() ) {
			cout << "cannot open logfile stream. " << endl ; 
			exit (-1) ; 
		}
		//microsecond timing
		char buffer[30];
 		struct timeval tv;
		time_t curtime;
		gettimeofday(&tv, NULL); 
  		curtime=tv.tv_sec;
 		strftime(buffer,30,"%m-%d-%Y  %T.",localtime(&curtime));

		//char strTime[20]; 
		//time_t time1 = time(NULL) ; 
		//tm * time2 = localtime ( &time1 ) ; 
		//strftime( &strTime[0], 20, "%Y-%m-%dT%H:%M:%S", time2 ) ; 
		char hostname[20] ; 
		gethostname(hostname, 20) ; 

		logstream << "<?xml version=\"1.0\" encoding=\"ascii\"?> " << endl ; 
		logstream << "<log node=\"" << hostname << "\" date=\"" 
			<< buffer << tv.tv_usec << "\">" << endl ; 
	}
	else {
		logToFile = false;
		/**Added by Jared on 6/26/08. Corrected this so it will log to the console.
		 * As a note, printing to the console will effect efficiency so this option
		 * should be used with caution.
		 **/
		logstream.copyfmt(std::cout);
		logstream.clear(std::cout.rdstate());
		logstream.basic_ios<char>::rdbuf(std::cout.rdbuf());
		//edits end 
		char strTime[20]; 
		time_t time1 = time(NULL) ; 
		tm * time2 = localtime ( &time1 ) ; 
		strftime( &strTime[0], 20, "%Y-%m-%dT%H:%M:%S", time2 ) ; 
		char hostname[20] ; 
		gethostname(hostname, 20) ; 

		logstream << "<?xml version=\"1.0\" encoding=\"ascii\"?> " << endl ; 
		logstream << "<log node=\"" << hostname << "\" date=\"" 
			<< strTime << "\">" << endl ; 		
	}
}

/**Print a string to the log file or console
 **/
void Logger::Log ( string line ) {
	pthread_mutex_lock (&mutex) ; 
	if ( logToFile ) {
		if (!logstream.bad() ) 
			logstream << "   " << line << endl ; 	
		else {
			cout << "error logging to file." << endl ;
		}
	}
	else
		cout << "   " << line << endl ; 
	pthread_mutex_unlock(&mutex) ; 
}

/**Close Log
 **/
void Logger::CloseLog ( ) {
	logstream.close() ; 
}
