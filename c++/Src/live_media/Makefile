# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

NAME = liblive_media
STATICLIB = liblive_media.a
SHAREDLIB = liblive_media.so
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
CCFLAGS += -I../live_common
CCFLAGS += -I../live_logging
CCLIBS	+= -fPIC

C++FLAGS = $(CCFLAGS)

CFILES =

CPPFILES =	AACUtility.cpp \
			AVCConfig.cpp \
			BitReader.cpp \
			BitWriter.cpp \
			h264.cpp \
			h264_slice.cpp \
			h264_sps.cpp \
			H264Utility.cpp 

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
	cp -f AACUtility.h ../../linux/inc/
	cp -f AVCConfig.h ../../linux/inc/
	cp -f BitReader.h ../../linux/inc/
	cp -f BitWriter.h ../../linux/inc/
	cp -f H264Utility.h ../../linux/inc/

install: $(STATICLIB)
clean:
	rm -f $(STATICLIB) $(SHAREDLIB) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

.SUFFIXES: .cpp .c .o

.cpp.o:
	$(C++) -c -o $*.o $(DEFINES) $(C++FLAGS) $*.cpp

.c.o:
	$(CC) -c -o $*.o $(DEFINES) $(CCFLAGS) $*.c
	

