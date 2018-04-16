STRIP=$(CROSS)-strip --remove-section=.note --remove-section=.comment
#CC = g++
CXXFLAGS = -W -Wall -g -std=c++11 -I../libSysCommon/include

LIBS   += -L../libSysCommon -lcommonLib

MONITOR=monitor
MONITOR_OBJS=monitorTask.o


ifeq ($(DEBUG),1)
	CXXFLAGS+=-DDEBUG
endif

all: $(MONITOR)

$(MONITOR):$(MONITOR_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(MONITOR_OBJS) $(LIBS)


clean:
	rm -rf $(MONITOR)
	rm -rf *.o	

