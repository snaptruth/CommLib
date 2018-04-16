BIN	 =	bnlog
#CC       =	g++
LIBS     =      -L../libSysCommon -L../libtinyXml -lcommonLib -lpthread -lrt -ldl -lbntinyxml 
CIFLAGS	 =      -I../libSysCommon/include -I./include -I../libtinyXml/include
CXXFLAGS =  -Wall -g -std=c++11 $(CIFLAGS)

ROOTSRC  =	$(wildcard ./*.cpp) $(wildcard ./src/*.cpp)
ROOTOBJ	 =	$(patsubst %cpp,%o,$(ROOTSRC))

#SUBDIR   = $(shell ls -d ./tinyxml/*.cpp)
#SUBSRC   = $(shell find $(SUBDIR) -name '*.cpp')
#SUBOBJ   = $(patsubst %cpp,%o,$(SUBSRC))

$(BIN):$(ROOTOBJ)
	$(CXX) $(CXXFLAGS) $(ROOTOBJ) -o $(BIN) $(LIBS)

.PHONY:clean 	
clean:
	rm -rf $(ROOTOBJ) $(BIN)
