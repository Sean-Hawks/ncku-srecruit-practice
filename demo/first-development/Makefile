CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra

.PHONY: all check clean
all: bin/worker

bin/worker: worker.c
	@mkdir -p bin
	$(CC) $(CFLAGS) worker.c -o bin/worker

check: bin/worker
	python3 host.py --check

clean:
	rm -f bin/worker
