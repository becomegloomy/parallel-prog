CC       = gcc
CFLAGS   = -O2 -fopenmp -Wall -Wextra -std=c11
LDLIBS   = -lm

IPP_INC  = /opt/intel/oneapi/ipp/latest/include
IPP_LIB  = /opt/intel/oneapi/ipp/latest/lib/intel64

SRC_DIR  = src
BIN_SEQ  = ipp-parallel
BIN_PAR  = omp-parallel

.PHONY: all clean bench overhead plots

all: $(BIN_SEQ) $(BIN_PAR)

$(BIN_SEQ): $(SRC_DIR)/ipp-parallel.c
	$(CC) $(CFLAGS) -I$(IPP_INC) $< -L$(IPP_LIB) -lipps -lippcore $(LDLIBS) -o $@

$(BIN_PAR): $(SRC_DIR)/omp-parallel.c
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

bench: $(BIN_SEQ)
	./scripts/run_all.sh

overhead: $(BIN_PAR)
	./scripts/run_overhead.sh

plots:
	python3 scripts/plot_results.py
	python3 scripts/plot_results2.py

clean:
	rm -f $(BIN_SEQ) $(BIN_PAR)
