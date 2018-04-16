BIN	= 	libbntinyxml.so
INC	=	-I./include
LIB	= 	-lpthread -lrt -ldl
#CC	=	g++
CXXFLAGS  =       -Wall -g -std=c++11 $(INC)

SRC     =	$(wildcard ./src/*.cpp)

$(BIN):
		#$(CC) $(CFLAGS) -shared -o $@ $(OBJ) $(LIB)
		$(CXX) $(CXXFLAGS) -shared -fpic -o $(BIN) $(SRC) $(LIB)		
.PRONY:clean
clean:
		@echo "Removing linked and compiled files......"
		rm -f $(BIN)
