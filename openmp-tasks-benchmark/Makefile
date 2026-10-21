CC       = gcc
CFLAGS   = -O2 -fopenmp -Wall -Wextra -std=c11
LDLIBS   = -lm

PYTHON  ?= python3
SRC_DIR  = src
BIN      = omp-tasks-bench

.PHONY: all clean bench plots

all: $(BIN)

$(BIN): $(SRC_DIR)/omp-tasks-bench.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

bench: $(BIN)
	bash scripts/run.sh ./$(BIN)

plots:
	$(PYTHON) scripts/plot.py

clean:
	rm -f $(BIN) results_2.csv *.png
