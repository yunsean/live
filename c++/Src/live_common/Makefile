# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  

NAME = liblive_common
STATICLIB = liblive_common.a
SHAREDLIB = liblive_common.so
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
CCLIBS	+= -fPIC

C++FLAGS = $(CCFLAGS)

CFILES =

CPPFILES =	SmartBlk.cpp \
			ShareBlk.cpp \
			SmartErr.cpp \
			BitStream.cpp \
			ByteStream.cpp \
			Profile.cpp \
			Byte.cpp \
			file.cpp \
			net.cpp \
			xutils.cpp

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
	cp -f SmartPtr.h ../../linux/inc/
	cp -f SmartArr.h ../../linux/inc/
	cp -f SmartHdr.h ../../linux/inc/
	cp -f SmartRet.h ../../linux/inc/
	cp -f SmartBlk.h ../../linux/inc/
	cp -f SmartLck.h ../../linux/inc/
	cp -f SmartErr.h ../../linux/inc/
	cp -f SmartNal.h ../../linux/inc/
	cp -f ShareBlk.h ../../linux/inc/
	cp -f Byte.h ../../linux/inc/
	cp -f ByteStream.h ../../linux/inc/
	cp -f BitStream.h ../../linux/inc/
	cp -f Profile.h ../../linux/inc/
	cp -f xchar.h ../../linux/inc/
	cp -f xevent.h ../../linux/inc/
	cp -f xmember.h ../../linux/inc/
	cp -f xsemaphore.h ../../linux/inc/
	cp -f xstring.h ../../linux/inc/
	cp -f xsystem.h ../../linux/inc/
	cp -f xthread.h ../../linux/inc/
	cp -f xexception.h ../../linux/inc/
	cp -f xaid.h ../../linux/inc/
	cp -f xutils.h ../../linux/inc/
	cp -f os.h ../../linux/inc/
	cp -f net.h ../../linux/inc/
	cp -f file.h ../../linux/inc/
	cp -f ierror.h ../../linux/inc/
	cp -f wingetopt.h ../../linux/inc/

install: $(STATICLIB)
clean:
	rm -f $(STATICLIB) $(SHAREDLIB) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

.SUFFIXES: .cpp .c .o

.cpp.o:
	$(C++) -c -o $*.o $(DEFINES) $(C++FLAGS) $*.cpp

.c.o:
	$(CC) -c -o $*.o $(DEFINES) $(CCFLAGS) $*.c
	

