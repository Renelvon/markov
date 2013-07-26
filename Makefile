CC=g++
CCFLAGS= -Wall -Wextra -std=c++11 -O2 -march=native

.PHONY: clean distclean

all: main

main: main.cpp
	$(CC) $(CCFLAGS) $^ -o $@

clean:
	$(RM) *.o

distclean: clean
	$(RM) main
