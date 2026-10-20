CC       = gcc
CFLAGS   = -O2 -fopenmp -Wall -Wextra -std=c11
LDLIBS   = -lm

SRC_DIR  = src
BIN      = omp-parallel-stages

.PHONY: all clean bench cpu plots

all: $(BIN)

$(BIN): $(SRC_DIR)/omp-parallel-stages.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

bench: $(BIN)
	python3 scripts/speedup.py

cpu: $(BIN)
	python3 scripts/cpu.py

plots:
	python3 scripts/plot_results.py

clean:
	rm -f $(BIN) results.csv cpu_usage.csv *.png
