target: decide

CFLAGS=-g -O3 -fopenmp -std=c99

main.o: main.c projet.h
aux.o: aux.c projet.h


decide: main.o aux.o
	$(CC) $(CFLAGS) $^ -o $@

exec: decide
	export OMP_NUM_THREADS=2 &&./decide "4k//4K/4P w"

clean:
	rm -f *.o decide

.PHONY: clean
