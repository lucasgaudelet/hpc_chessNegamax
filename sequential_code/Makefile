all: target

target: decide

CFLAGS=-g -O3 -std=c99
CC=mpicc

main.o: main.c projet.h
aux.o: aux.c projet.h
main_timers: main_timers.c projet.h

decide: main.o aux.o
	$(CC) $(CFLAGS) $^ -o $@

decide_timers: main_timers.o aux.o
	$(CC) $(CLFAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f *.o decide decide_timers

