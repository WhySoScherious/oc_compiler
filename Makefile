# Author: Paul Scherer, pscherer@ucsc.edu

GPP   = g++ -g -O0 -Wall -Wextra -std=gnu++0x
GRIND = valgrind --leak-check=full --show-reachable=yes

CHEADER = stringset.h
CSOURCE = main.cc stringset.cc
OBJECTS = ${CSOURCE:.cc=.o}
EXECBIN = oc
SOURCES = ${CHEADER} ${CSOURCE} Makefile README oclib.oh
SUBMIT = submit cmps104a-wm.f13 asg1

all : oc

oc : main.o stringset.o
	${GPP} main.o stringset.o -o oc

%.o : %.cc
	${GPP} -c $<

check :
	- checksource ${SOURCES}

spotless : clean
	- rm cpp_output *.str test.out ${OBJECTS}

clean :
	- rm ${EXECBIN}

grind : oc
	${GRIND} oc 01-hello.oc >test.out 2>&1

lis : test.out
	mkpspdf Listing.ps stringset.h stringset.cc main.cc \
	        Makefile test.out

submit : ${SOURCES}
	${SUBMIT} ${SOURCES}

# Depencencies.
main.o: main.cc stringset.h
stringset.o: stringset.cc stringset.h

