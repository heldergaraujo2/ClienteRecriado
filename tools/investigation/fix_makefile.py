# Ciclo: ciclo13 item1
# Pergunta: Fix Makefile test-san 389 vs 469 discrepância, sanitização cobrir 100% checks
# Evidência: docs/evidence/ciclo13_test_san_full.log
# Origem: /tmp/fix_makefile.py movido para backup permanente ciclo15 Parte A
# Categoria: (a) FERRAMENTA/SCRIPT DE DESCOBERTA ÚNICO
# Data backup: 2026-10-12
# Branch: arena/519f8fc9-clienterecriado

path="Makefile"
text=open(path).read()
# Replace test-san section
old="""# A separate binary rather than a flag on the normal one, so `make test` can
# never silently run an instrumented build, and the documented sanitizer claim
# has a target behind it that anyone can re-run.
SAN_FLAGS := -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
SAN_BUILD := build-san

test-san: $(SAN_BUILD)/test_assets
\t$(SAN_BUILD)/test_assets

$(SAN_BUILD)/test_assets: $(TEST_SRC) $(LIB_SRCS)
\t@mkdir -p $(SAN_BUILD)
\t$(CXX) -std=c++17 -Wall -Wextra $(SAN_FLAGS) -Isrc $^ -o $@
"""
new="""# A separate binary rather than a flag on the normal one, so `make test` can
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
\t$(SAN_BUILD)/test_assets

$(SAN_BUILD)/test_assets: $(SAN_TEST_OBJ) $(SAN_BUILD)/libmuassets.a
\t$(CXX) $(SAN_FLAGS) $^ -o $@ $(LDFLAGS)

$(SAN_BUILD)/libmuassets.a: $(SAN_LIB_OBJS)
\t@mkdir -p $(dir $@)
\tar rcs $@ $(SAN_LIB_OBJS)

# Compile lib objects with sanitizers plus same warnings as normal build
$(SAN_BUILD)/%.o: %.cpp Makefile
\t@mkdir -p $(dir $@)
\t$(CXX) $(CXXSTD) $(SAN_FLAGS) $(WARN) -Isrc $(DEPFLAGS) -c $< -o $@

# Ensure Makefile changes force rebuild
$(SAN_BUILD)/$(TEST_SRC:%.cpp=%.o): $(TEST_SRC) Makefile
\t@mkdir -p $(dir $@)
\t$(CXX) $(CXXSTD) $(SAN_FLAGS) $(WARN) -Isrc $(DEPFLAGS) -c $< -o $@
"""
if old in text:
    text=text.replace(old,new)
    print("fixed makefile")
else:
    print("old not found, trying manual")
    # fallback replace via markers
    import re
    text=re.sub(r"SAN_FLAGS.*?\n\nclean:", new+"\n\nclean:", text, flags=re.S)
open(path,'w').write(text)
