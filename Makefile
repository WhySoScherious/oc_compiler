# Paul Scherer, pscherer@ucsc.edu

MKFILE    = Makefile
DEPSFILE  = ${MKFILE}.deps
NOINCLUDE = ci clean spotless
NEEDINCL  = ${filter ${NOINCLUDE}, ${MAKECMDGOALS}}
VALGRIND  = valgrind --leak-check=full --show-reachable=yes

#
# Definitions of list of files:
#
HSOURCES  = astree.h  lyutils.h  auxlib.h  stringset.h symtable.h \
            typecheck.h oilprint.h
CSOURCES  = astree.cc lyutils.cc auxlib.cc stringset.cc main.cc \
            symtable.cc typecheck.cc oilprint.cc
LSOURCES  = scanner.l
YSOURCES  = parser.y
ETCSRC    = oclib.oh oclib.c README Makefile 
CLGEN     = yylex.cc
HYGEN     = yyparse.h
CYGEN     = yyparse.cc
CGENS     = ${CLGEN} ${CYGEN}
ALLGENS   = ${HYGEN} ${CGENS}
EXECBIN   = oc
ALLSRCF   = ${HSOURCES} ${CSOURCES} ${LSOURCES} ${YSOURCES} ${ETCSRC}
ALLCSRC   = ${CSOURCES} ${CGENS}
OBJECTS   = ${ALLCSRC:.cc=.o}
LREPORT   = yylex.output
YREPORT   = yyparse.output
REPORTS   = ${LREPORT} ${YREPORT}
ALLSRC    = ${ETCSRC} ${YSOURCES} ${LSOURCES} ${HSOURCES} ${CSOURCES}
TESTINS   = ${wildcard test?.oc}
LISTSRC   = ${ALLSRC} ${HYGEN}
SUBMIT = submit cmps104a-wm.f13 asg5

#
# Definitions of the compiler and compilation options:
#
GCC       = g++ -g -O0 -Wall -Wextra -std=gnu++0x
MKDEPS    = g++ -MM -std=gnu++0x

#
# The first target is always ``all'', and hence the default,
# and builds the executable images
#
all : ${EXECBIN}

#
# Build the executable image from the object files.
#
${EXECBIN} : ${OBJECTS}
	${GCC} -o${EXECBIN} ${OBJECTS}

#
# Build an object file form a C source file.
#
%.o : %.cc
	${GCC} -c $<


#
# Checksource files.
#
check :
	- checksource ${ALLSRCF}

#
# Submit the required assignment files.
#
submit :
	- ${SUBMIT} ${ALLSRCF}

#
# Build the scanner.
#
${CLGEN} : ${LSOURCES}
	flex --outfile=${CLGEN} ${LSOURCES} 2>${LREPORT}
	- grep -v '^  ' ${LREPORT}

#
# Build the parser.
#
${CYGEN} ${HYGEN} : ${YSOURCES}
	bison --defines=${HYGEN} --output=${CYGEN} ${YSOURCES}

#
# Make a listing from all of the sources
#
lis : ${LISTSRC} tests
	mkpspdf List.source.ps ${LISTSRC}
	mkpspdf List.output.ps ${REPORTS} \
		${foreach test, ${TESTINS:.oc=}, \
		${patsubst %, ${test}.%, oc out err}}

#
# Clean and spotless remove generated files.
#
clean :
	- rm ${OBJECTS} ${ALLGENS} ${REPORTS} ${DEPSFILE} core
	- rm ${foreach test, ${TESTINS:.oc=}, \
		${patsubst %, ${test}.%, out err}}

spotless : clean
	- rm ${EXECBIN} *.str *.tok *.ast *.sym *.oil List.*.ps \
          List.*.pdf


#
# Build the dependencies file using the C preprocessor
#
deps : ${ALLCSRC}
	@ echo "# ${DEPSFILE} created `date` by ${MAKE}" >${DEPSFILE}
	${MKDEPS} ${ALLCSRC} >>${DEPSFILE}

${DEPSFILE} :
	@ touch ${DEPSFILE}
	${MAKE} --no-print-directory deps

#
# Test
#

tests : ${EXECBIN}
	touch ${TESTINS}
	make --no-print-directory ${TESTINS:.oc=.out}

%.out %.err : %.oc ${EXECBIN}
	( ${VALGRIND} ${EXECBIN} -ly -@@ $< \
	;  echo EXIT STATUS $$? 1>&2 \
	) 1>$*.out 2>$*.err

#
# Everything
#
again :
	gmake --no-print-directory spotless deps ci all lis
	
ifeq "${NEEDINCL}" ""
include ${DEPSFILE}
endif

