#
CXX = g++
CXXFLAGS = -g -Wall -Werror -std=gnu++17 -D_GNU_SOURCE

# .cpp	(.cc/.cxx/.C)
# .h	(.hh/-)a

all: mgt

mgt: mgt.o parser.o
	$(CXX) mgt.o parser.o -o mgt

mgt.o: mgt.cpp mgt.h

parser.o: parser.cpp mgt.h

clean:
	rm -f *o
	rm -f mgt
	rm -f *~
	rm -f .*~
