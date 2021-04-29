#
CXX = g++
CXXFLAGS = -g -Wall -Werror -std=gnu++17 -D_GNU_SOURCE

# .cpp	(.cc/.cxx/.C)
# .h	(.hh/-)a

all: mgt.o parser.o server.o
	$(CXX) server.o mgt.o parser.o -o server

mgt.o: mgt.cpp mgt.h

server.o: server.cpp mgt.h

parser.o: parser.cpp mgt.h

clean:
	rm -f *o
	rm -f server
	rm -f *~
	rm -f .*~

