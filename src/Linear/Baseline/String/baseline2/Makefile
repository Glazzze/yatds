CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
OPTFLAGS = -O2

SRC      = src
CORE     = $(SRC)/pattern_match/match.cpp
HEADER   = $(SRC)/pattern_match/match.hpp

TEST_SRCS   = $(SRC)/task4_command_test/main_test.cpp $(CORE)
TEST_TARGET = test

UNIT_TARGET  = /tmp/string_b2_unit
NAIVE_TARGET = /tmp/string_b2_naive
NEXT_TARGET  = /tmp/string_b2_nextval

all: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS) $(HEADER)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(TEST_SRCS) -o $(TEST_TARGET)

unit: $(UNIT_TARGET)
	$(UNIT_TARGET)

$(UNIT_TARGET): $(SRC)/task2_kmp/main_unit.cpp $(CORE) $(HEADER)
	$(CXX) $(CXXFLAGS) $^ -o $@

bench: $(NAIVE_TARGET) $(NEXT_TARGET)
	@echo "===== Step 1: 朴素算法最坏情况 ====="
	$(NAIVE_TARGET)
	@echo "===== Step 3: nextval 优化对比 ====="
	$(NEXT_TARGET)

$(NAIVE_TARGET): $(SRC)/task1_naive_analysis/main_naive.cpp $(CORE) $(HEADER)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(NEXT_TARGET): $(SRC)/task3_nextval/main_nextval.cpp $(CORE) $(HEADER)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

check: $(TEST_TARGET)
	./$(TEST_TARGET)

ASAN_TEST = /tmp/string_b2_asan_test
ASAN_UNIT = /tmp/string_b2_asan_unit
asan:
	$(CXX) $(CXXFLAGS) -fsanitize=address,leak $(TEST_SRCS) -o $(ASAN_TEST)
	$(CXX) $(CXXFLAGS) -fsanitize=address,leak $(SRC)/task2_kmp/main_unit.cpp $(CORE) \
		-o $(ASAN_UNIT)
	$(ASAN_UNIT)
	$(ASAN_TEST)

clean:
	rm -f $(TEST_TARGET) $(UNIT_TARGET) $(NAIVE_TARGET) $(NEXT_TARGET) $(ASAN_TEST) \
		$(ASAN_UNIT)

.PHONY: all unit bench check asan clean