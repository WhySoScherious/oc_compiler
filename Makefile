# Author: Paul Scherer, pscherer@ucsc.edu

GPP   = g++ -g -O0 -Wall -Wextra -std=gnu++0x
GRIND = valgrind --leak-check=full --show-reachable=yes


all : oc

oc : main.o stringset.o
	${GPP} main.o stringset.o -o oc

%.o : %.cc
	${GPP} -c $<

spotless : clean
	- rm oc cpp_output

clean :
	-rm stringset.o main.o

test.out : oc
	${GRIND} oc 01-hello.oc >test.out 2>&1

lis : test.out
	mkpspdf Listing.ps stringset.h stringset.cc main.cc \
	        Makefile test.out

# Depencencies.
main.o: main.cc stringset.h
stringset.o: stringset.cc stringset.h

