SRC = src/*.cpp

all:
	g++ $(SRC) -Iinclude -o slfr
