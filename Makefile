CC ?= cc
PKG_CONFIG ?= pkg-config

RAYLIB_CFLAGS := $(shell $(PKG_CONFIG) --cflags raylib 2>/dev/null)
RAYLIB_LIBS := $(shell $(PKG_CONFIG) --libs raylib 2>/dev/null)

CPPFLAGS += -Iinclude -D_POSIX_C_SOURCE=200809L \
	'-DSP_DEFAULT_ATOMS_DIR="$(CURDIR)/assets/atoms"'
CFLAGS += -std=c11 -O2 -g -Wall -Wextra -Wpedantic -MMD -MP
LDLIBS += $(RAYLIB_LIBS) -lm

BUILD := build
BIN := $(BUILD)/snake_puzzle
TEST_BIN := $(BUILD)/tests

CORE_SRC := $(wildcard src/core/*.c) $(wildcard src/entity/*.c)
GEN_SRC := $(wildcard src/gen/*.c)
APP_SRC := $(wildcard src/app.c)
UI_SRC := $(wildcard src/ui/*.c)
MAIN_SRC := $(wildcard src/main.c)
PLATFORM_SRC := src/platform.c
ALL_SRC := $(CORE_SRC) $(GEN_SRC) $(APP_SRC) $(UI_SRC) $(MAIN_SRC) $(PLATFORM_SRC)
OBJ := $(patsubst %.c,$(BUILD)/obj/%.o,$(ALL_SRC))
DEP := $(OBJ:.o=.d)

TEST_SRC := $(wildcard tests/*.c)
TEST_CORE_SRC := $(CORE_SRC) $(GEN_SRC) $(APP_SRC) $(PLATFORM_SRC)

.PHONY: all run test check clean tools-test validate-atoms batch-example

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(OBJ) $(LDLIBS) -o $@

$(BUILD)/obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RAYLIB_CFLAGS) -c $< -o $@

$(TEST_BIN): $(TEST_SRC) $(TEST_CORE_SRC)
	@mkdir -p $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SRC) $(TEST_CORE_SRC) -lm -o $@

run: $(BIN)
	./$(BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

check: test tools-test

tools-test: $(BIN)
	python3 tools/validate_atoms.py
	python3 tools/export_atoms.py --output $(BUILD)/atoms.spa
	$(BIN) --atoms $(BUILD)/atoms.spa --batch 20 --seed 7000 >/dev/null

validate-atoms:
	python3 tools/validate_atoms.py

batch-example: $(BIN)
	python3 tools/batch_sim.py --count 100 --seed 1000

clean:
	rm -rf $(BUILD)

-include $(DEP)
