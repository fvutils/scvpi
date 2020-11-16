#!/bin/sh

SYSTEMC=`pwd`/../../systemc
PATH=`pwd`/../packages/python/bin:$PATH

cocotb_prefix=`cocotb-config --prefix`
vpi=${cocotb_prefix}/cocotb/libs/libcocotbvpi_modelsim.so


g++ -I${SYSTEMC}/include -I. -export-dynamic \
	-I../src ../src/scvpi.cpp \
	test.cpp -L${SYSTEMC}/lib-linux64 -lsystemc -o test -ldl
if test $? -ne 0; then exit 1; fi

export LD_LIBRARY_PATH=${SYSTEMC}/lib-linux64:${LD_LIBRARY_PATH}

export MODULE=tb
./test +vpi=${vpi} $*
# valgrind --tool=memcheck ./test +vpi=${vpi} $*

