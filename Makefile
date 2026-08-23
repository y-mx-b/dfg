CC=clang
CFLAGS=-std=c11 -Wall -Werror
SRC=$(wildcard src/*.c)

dfg: $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f dfg 2>/dev/null
