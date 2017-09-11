TERGET		= gateway

CXX			= g++ -g -Wall -D_REENTRANT -D__USE_POSIX -D__linux__ -D__PTHREADS_MUTEXES__ 
CC			= 
CXXFLAGS	= -std=c++11
CXXFLAGS	+= -I../../linux/inc/ 
CXXFLAGS	+= -I../live_input -I../live_output

CXXLIBS		+= -L../../linux/lib -llive_network -llive_media -llive_input -llive_output -llive_vpf -llive_logging -llive_common -llive_literal
CXXLIBS		+= -L../../linux/lib -levent -levent_core -levent_pthreads
CXXLIBS		+= -L./usr/local/lib -lpthread -ldl
OBJES		=	live_gateway.o \
				SourceFactory.o \
				EventBase.o \
				GatewayServer.o \
				GatewayClient.o \
				Source.o \
				SystemInfo.o \
				Channel.o \
				ChannelFactory.o \
				SinkFactory.o

SUBOBJES	= 
SUBDIRS		= 

.PHONY: subdirs $(SUBDIRS) $(TERGET)
subdirs: $(SUBDIRS) $(TERGET) 
$(SUBDIRS):
	$(MAKE) -C $@

#all: $(TERGET) $(VOD)


#echo -n $__TIMES__
	
$(TERGET):$(OBJES)
	$(CXX) -o $(TERGET) $(CXXFLAGS) $(OBJES)  $(SUBOBJES) $(CXXLIBS) 

   
$(OBJES) : %.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@


clean: cleandir
	rm -f $(TERGET) *.o *.TMP *~
    
clear: cleardir
	rm -f *.o *.TMP *~ 
    
cleandir:
	for dir in $(SUBDIRS);do \
$(MAKE) -C $$dir clean; \
done

cleardir:
	for dir in $(SUBDIRS);do \
$(MAKE) -C $$dir clear; \
done


