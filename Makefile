CC = cc
CPPFLAGS = -Iinclude
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -g
SOURCES = src/main.c src/state.c src/input.c src/avoidance.c src/detection.c src/recovery.c
HEADERS = include/state.h include/input.h include/modules.h

.PHONY: all run test clean
all: build/deadlock_manager
build:
	mkdir -p build
build/deadlock_manager: $(SOURCES) $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@
run: all
	./build/deadlock_manager
build/test_state: tests/test_state.c src/state.c src/input.c $(HEADERS) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_state.c src/state.c src/input.c -o $@
test: all build/test_state
	./build/test_state
	python3 tests/test_cli.py
clean:
	rm -rf build
