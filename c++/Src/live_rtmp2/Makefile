# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

NAME = liblive_rtmp2
STATICLIB = liblive_rtmp2.a
SHAREDLIB = liblive_rtmp2.so
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
CCFLAGS	+= -I../../linux/inc/
CCLIBS	+= -fPIC

CXXLIBS		+= -L../../linux/lib -llive_logging -llive_common 
CCFLAGS		+= -DNO_CRYPTO
C++FLAGS	= $(CCFLAGS)

CFILES = rtmp.c \

CPPFILES =	amf.cpp \
			log.cpp \
			Metadata.cpp \
			parseurl.cpp \
			Publisher.cpp \
			Rtmp2Publisher.cpp

all: $(SHAREDLIB) $(STATICLIB)

$(SHAREDLIB): $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
	g++ -shared -fPIC -o $(SHAREDLIB) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(CCLIBS)
	mkdir -p ../../linux/bin
	cp -f $(SHAREDLIB) ../../linux/bin

$(STATICLIB): $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
	ar -r $(STATICLIB) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
	ranlib $(STATICLIB)
	mkdir -p ../../linux/lib
	cp -f $(STATICLIB) ../../linux/lib
	mkdir -p ../../linux/inc
	cp -f Publisher.h ../../linux/inc/

install: $(STATICLIB)
clean:
	rm -f $(STATICLIB) $(SHAREDLIB) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

.SUFFIXES: .cpp .c .o

.cpp.o:
	$(C++) -c -o $*.o $(DEFINES) $(C++FLAGS) $*.cpp

.c.o:
	$(CC) -c -o $*.o $(DEFINES) $(CCFLAGS) $*.c
	

