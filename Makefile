CC      ?= cc
CFLAGS  ?= -O2 -std=c99 -Wall -Wextra -pedantic

all: libpawl.a

libpawl.a: pawl.o
	ar rcs $@ $^

pawl.o: pawl.c pawl.h
	$(CC) $(CFLAGS) -c pawl.c

# C library against the Python reference, then the self-test
check: pawl.o
	$(CC) $(CFLAGS) -o test_vectors test_vectors.c pawl.o
	./test_vectors > vectors_c.out
	python3 compare_vectors.py > vectors_py.out
	cmp vectors_c.out vectors_py.out
	cmp vectors_c.out vectors.txt
	$(CC) $(CFLAGS) -o tools/selftest tools/selftest.c pawl.o -lm
	./tools/selftest
	@echo "all checks passed"

# endless byte stream for test batteries, e.g.  ./tools/pawl_stream gen | RNG_test stdin64
tools: pawl.o
	$(CC) $(CFLAGS) -o tools/pawl_stream tools/pawl_stream.c pawl.o
	$(CC) $(CFLAGS) -o tools/bench tools/bench.c pawl.o

clean:
	rm -f *.o *.a *.out test_vectors tools/selftest tools/pawl_stream tools/bench

.PHONY: all check tools clean
