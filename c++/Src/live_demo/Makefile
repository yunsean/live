TERGET		= demo

CXX			= g++ -g -Wall -D_REENTRANT -D__USE_POSIX -D__linux__ -D__PTHREADS_MUTEXES__ 
CC			= 
CXXFLAGS	= -std=c++11
CXXFLAGS	+= -I../../linux/inc/ -I../../linux/inc/libz/

CXXLIBS		= -L./usr/local/lib -lpthread 
CXXLIBS		+= -L../../linux/lib -llive_media -llive_vpf -llive_logging -llive_common
CXXLIBS		+= -L../../linux/lib -levent -levent_core -lz
OBJES		=  live_demo.o

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


