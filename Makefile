CXX=clang++
CC=clang
CFLAGS=-g3 -Wall -fsanitize=address
LDFLAGS=-lclang-cpp -lLLVM

SRC_FILES=$(wildcard *.cpp)
BIN_FILES=$(subst .cpp,.out,$(SRC_FILES))

all: $(BIN_FILES)

%.out: %.cpp
	$(CXX) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f *.out *.o *.s
