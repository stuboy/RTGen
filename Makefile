COMPILER=g++
FLAGS= -pthread 
HEADERS=packet.h packetInfo.h streamInfo.h logger.h

all: generator receiver

streamInfo.o: streamInfo.h streamInfo.cpp packetInfo.h logger.o
	$(COMPILER) $(FLAGS) -c -o streamInfo.o streamInfo.cpp 
	
logger.o: logger.h logger.cpp
	$(COMPILER) $(FLAGS) -c -o logger.o logger.cpp 

generator: generator.cpp packet.h
	$(COMPILER) $(FLAGS) -o generator generator.cpp 
	
receiver: $(HEADERS) receiver.cpp streamInfo.o logger.o
	$(COMPILER) $(FLAGS) -o receiver receiver.cpp streamInfo.o logger.o

clean: 
	rm -f generator receiver *.o
