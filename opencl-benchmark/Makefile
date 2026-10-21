CC       = gcc
CFLAGS   = -O2 -Wall -Wextra -std=c11
LDLIBS   = -lm -lOpenCL

PYTHON  ?= python3
SRC_DIR  = src
BIN      = opencl-bench

.PHONY: all clean bench plots

all: $(BIN)

$(BIN): $(SRC_DIR)/opencl-bench.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

bench: $(BIN)
	$(PYTHON) scripts/run.py

plots:
	$(PYTHON) scripts/plot.py

clean:
	rm -f $(BIN) results.csv *.png
