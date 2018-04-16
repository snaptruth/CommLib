###########################################
#Makefile for simple programs
###########################################
VPATH = src
INC= -I./include 
LIB= -lpthread -lrt -ldl 
#CC=g++
CXXFLAGS= -Wall -g -std=c++11 $(INC) -fPIC -D_GNU_SOURCE

PRG=libcommonLib.so
OBJ=taskLib.o monitor.o


all: $(PRG)
	
$(PRG): $(OBJ)
	$(CXX) $(CXXFLAGS) -shared -o $@ $(OBJ) $(LIB)


.PRONY:clean
clean:
	@echo "Removing linked and compiled files......"
	rm -f $(OBJ) $(PRG) 

