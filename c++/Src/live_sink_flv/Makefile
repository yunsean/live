# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

NAME = liblive_sink_flv
STATICLIB = liblive_sink_flv.a
SHAREDLIB = liblive_sink_flv.so
C++ = gcc #$(CPLUS)
CC = gcc #$(CCOMP)
LINK = $(LINKER)

COMPILER_FLAGS = -D_REENTRANT -D__USE_POSIX -D__linux__ -pipe
INCLUDE_FLAG = -include
CCFLAGS += $(COMPILER_FLAGS) -fPIC -g -Wall -Wno-deprecated -std=c++11

# OPTIMIZATION
#CCFLAGS += -O2

# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS

CCFLAGS += -I.
CCFLAGS += -I..
CCFLAGS += -I../../linux/inc
CCFLAGS += -I../live_output

C++FLAGS = $(CCFLAGS)

CXXLIBS		+= -L../../linux/lib -llive_vpf -llive_input -llive_logging -llive_media -llive_common -llive_literal
CXXLIBS		+= -L../../linux/lib -levent -levent_core -levent_pthreads
CXXLIBS		+= -L./usr/local/lib -lpthread -ldl -fPIC

CFILES =

CPPFILES =	live_sink_flv.cpp \
			FlvFactory.cpp \
			FlvWriter.cpp

all: $(SHAREDLIB) $(STATICLIB)

#-shared 
$(SHAREDLIB): $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
	g++ -shared -fPIC -o $(SHAREDLIB) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(CXXLIBS) 
	mkdir -p ../../linux/bin
	cp -f $(SHAREDLIB) ../../linux/bin
	mkdir -p ../live_gateway/Plugin/sink/
	cp -f $(SHAREDLIB) ../live_gateway/Plugin/sink/

$(STATICLIB): $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
	ar -r $(STATICLIB) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
	ranlib $(STATICLIB)
	mkdir -p ../../linux/lib
	cp -f $(STATICLIB) ../../linux/lib
	mkdir -p ../../linux/inc

install: $(STATICLIB)
clean:
	rm -f $(STATICLIB) $(SHAREDLIB) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

.SUFFIXES: .cpp .c .o

.cpp.o:
	$(C++) -c -o $*.o $(DEFINES) $(C++FLAGS) $*.cpp

.c.o:
	$(CC) -c -o $*.o $(DEFINES) $(CCFLAGS) $*.c
	

