CXXFLAGS += -Wall -O2
LD = g++
CC = g++
CXX = g++

all: ascompare

cleanreport:
	rm -f *.html *.png

clean: cleanreport
	rm -f *~ *.o ascompare

dep:
	makedepend -I. -Y *.cpp
	-rm Makefile.bak
# DO NOT DELETE
