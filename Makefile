CXX = g++
SRC = src/*.cpp	
TARGET = slfr

all:
	$(CXX) $(SRC) -Iinclude -o $(TARGET)

clear:
	rm -f ./$(TARGET)
