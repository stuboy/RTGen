
#include <fstream>
#include <iostream>
#include <ostream>

#ifndef LOGGER_H
#define LOGGER_H

using namespace std; 

class Logger {
private: 
	ofstream logstream ; 
	bool logToFile ; 
	pthread_mutex_t mutex ; 
	
		
public: 
	Logger ( string filename ) ; 
	void Log ( string ) ; 
	void CloseLog ( ) ; 
};

#endif
