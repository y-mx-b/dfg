CC=clang
CFLAGS=-std=c11 -Wall -Werror -O2
SRC=$(wildcard src/*.c)
INSTALL_DIR=$(HOME)/.local/bin

dfg: $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean install uninstall

clean:
	rm -f dfg 2>/dev/null

install: dfg
	test -d "$(INSTALL_DIR)" || mkdir "$(INSTALL_DIR)"
	cp dfg "$(INSTALL_DIR)"

uninstall:
	test -e "$(INSTALL_DIR)/dfg" && rm "$(INSTALL_DIR)/dfg" || true
