# ClienteRecriado - build
#
# Deliberately has no third-party dependencies and no build-system download
# step. A C++17 compiler plus make is enough on Linux/macOS; the same sources
# build with MSVC or MinGW on Windows (see docs/BUILD.md).

CXX      ?= g++
CXXSTD   ?= -std=c++17
OPT      ?= -O2
WARN     := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
            -Wold-style-cast -Wuseless-cast -Wnon-virtual-dtor -Woverloaded-virtual \
            -Wcast-align -Wdouble-promotion -Wformat=2
CXXFLAGS ?= $(CXXSTD) $(OPT) $(WARN) -Isrc
LDFLAGS  ?=

# Track header dependencies. Without this, editing a header leaves object files
# compiled against the old struct layout linked against a library built with
# the new one - a silent ODR violation that shows up as a segfault long after
# the edit that caused it.
DEPFLAGS := -MMD -MP

BUILD    := build
LIB_SRCS := src/mu/assets/format.cpp \
            src/mu/assets/ozt.cpp \
            src/mu/assets/ozj.cpp \
            src/mu/assets/bmd.cpp \
            src/mu/assets/bmd_body.cpp \
            src/mu/assets/mesh.cpp \
            src/mu/render/cull.cpp \
            src/mu/render/buffers.cpp \
            src/mu/render/skinning.cpp \
            src/mu/platform/win32_window.cpp \
            src/mu/platform/main_loop.cpp \
            src/mu/assets/bmd_report.cpp \
            src/mu/assets/terrain.cpp \
            src/mu/assets/tga.cpp \
            src/mu/assets/manifest.cpp \
            src/mu/fs.cpp
LIB_OBJS := $(LIB_SRCS:%.cpp=$(BUILD)/%.o)

TOOL_SRC := tools/mutool.cpp
TEST_SRC := tests/test_assets.cpp
BENCH_SRC := bench/bench_assets.cpp

.PHONY: all test bench clean tool test-san

all: $(BUILD)/mutool $(BUILD)/test_assets

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD)/libmuassets.a: $(LIB_OBJS)
	@mkdir -p $(dir $@)
	ar rcs $@ $(LIB_OBJS)

$(BUILD)/mutool: $(BUILD)/$(TOOL_SRC:%.cpp=%.o) $(BUILD)/libmuassets.a
	$(CXX) $(CXXFLAGS) $< -L$(BUILD) -lmuassets -o $@ $(LDFLAGS)

$(BUILD)/test_assets: $(BUILD)/$(TEST_SRC:%.cpp=%.o) $(BUILD)/libmuassets.a
	$(CXX) $(CXXFLAGS) $< -L$(BUILD) -lmuassets -o $@ $(LDFLAGS)

$(BUILD)/bench_assets: $(BUILD)/$(BENCH_SRC:%.cpp=%.o) $(BUILD)/libmuassets.a
	$(CXX) $(CXXFLAGS) $< -L$(BUILD) -lmuassets -o $@ $(LDFLAGS)

tool: $(BUILD)/mutool
bench: $(BUILD)/bench_assets

test: $(BUILD)/test_assets
	$(BUILD)/test_assets

# A separate binary rather than a flag on the normal one, so `make test` can
# never silently run an instrumented build, and the documented sanitizer claim
# has a target behind it that anyone can re-run.
# FIX ciclo13 item1: test-san must compile and run the SAME set of checks as test.
# Cause of previous discrepancy 469 vs 389: old rule compiled all sources in one
# g++ invocation and relied only on file mtimes; adding new files to LIB_SRCS did
# not always trigger rebuild if mtime was older than existing binary, leading to
# stale binary with fewer checks. Fix: build objects in SAN_BUILD with same
# LIB_SRCS list, depend on Makefile, and use same warning set plus sanitizers.
SAN_FLAGS := -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
SAN_BUILD := build-san
SAN_LIB_OBJS := $(LIB_SRCS:%.cpp=$(SAN_BUILD)/%.o)
SAN_TEST_OBJ := $(SAN_BUILD)/$(TEST_SRC:%.cpp=%.o)

test-san: $(SAN_BUILD)/test_assets
	$(SAN_BUILD)/test_assets

$(SAN_BUILD)/test_assets: $(SAN_TEST_OBJ) $(SAN_BUILD)/libmuassets.a
	$(CXX) $(SAN_FLAGS) $^ -o $@ $(LDFLAGS)

$(SAN_BUILD)/libmuassets.a: $(SAN_LIB_OBJS)
	@mkdir -p $(dir $@)
	ar rcs $@ $(SAN_LIB_OBJS)

# Compile lib objects with sanitizers plus same warnings as normal build
$(SAN_BUILD)/%.o: %.cpp Makefile
	@mkdir -p $(dir $@)
	$(CXX) $(CXXSTD) $(SAN_FLAGS) $(WARN) -Isrc $(DEPFLAGS) -c $< -o $@

# Ensure Makefile changes force rebuild
$(SAN_BUILD)/$(TEST_SRC:%.cpp=%.o): $(TEST_SRC) Makefile
	@mkdir -p $(dir $@)
	$(CXX) $(CXXSTD) $(SAN_FLAGS) $(WARN) -Isrc $(DEPFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) $(SAN_BUILD)

# Pull in the generated header dependencies. The leading '-' keeps a fresh
# checkout working, since no .d files exist yet on the first build.
-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)
