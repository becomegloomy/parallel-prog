CC       = gcc
CFLAGS   = -O2 -Wall -Wextra -std=c11 -pthread
LDLIBS   = -lm -pthread

PYTHON  ?= python3
SRC_DIR  = src
BIN      = pthreads-bench

.PHONY: all clean bench plots

all: $(BIN)

$(BIN): $(SRC_DIR)/pthreads-bench.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

bench: $(BIN)
	$(PYTHON) scripts/run.py

plots:
	$(PYTHON) scripts/plot.py

clean:
	rm -f $(BIN) results.csv *.png
