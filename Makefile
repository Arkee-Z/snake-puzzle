CC ?= cc
CFLAGS ?= -std=c11 -O2 -g -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude
LDFLAGS ?=
LDLIBS ?=

ifeq ($(OS),Windows_NT)
EXEEXT := .exe
LDFLAGS += -static -static-libgcc
else
EXEEXT :=
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
endif

BUILD := build
SP_BIN := $(BUILD)/sp$(EXEEXT)
TEST_BIN := $(BUILD)/test_sp$(EXEEXT)

CORE_SRC := src/game.c
TERM_SRC := src/term.c
MAIN_SRC := src/main.c
TEST_SRC := tests/test_main.c

.PHONY: all run test clean

all: $(SP_BIN)

$(SP_BIN): $(MAIN_SRC) $(CORE_SRC) $(TERM_SRC) include/sp.h include/term.h
	@mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MAIN_SRC) $(CORE_SRC) $(TERM_SRC) $(LDFLAGS) $(LDLIBS) -o $@

$(TEST_BIN): $(TEST_SRC) $(CORE_SRC) include/sp.h
	@mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SRC) $(CORE_SRC) $(LDFLAGS) $(LDLIBS) -o $@

run: $(SP_BIN)
	./$(SP_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD)
