CC       = gcc
CFLAGS   = -O3 -Wall -Werror -lm
LDLIBS   = -lm
SRC      = auto-parallel.c
K_VALUES = 1 2 4 8

.PHONY: all clean seq par test

all: seq par

seq: lab1-seq

lab1-seq: $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

par: $(K_VALUES:%=lab1-par-%)

lab1-par-%: $(SRC)
	$(CC) $(CFLAGS) -floop-parallelize-all -ftree-parallelize-loops=$* -o $@ $< $(LDLIBS)

test: all
	./run_all.sh > sample_results.csv

clean:
	rm -f lab1-seq lab1-par-* sample_results.csv *.png
