CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
OPTFLAGS = -O2

SRC    = src
CORE   = $(SRC)/dna_search/dna_search.cpp
HEADER = $(SRC)/dna_search/dna_search.hpp

TEST_SRCS   = $(SRC)/task2_command_test/main_test.cpp $(CORE)
TEST_TARGET = test

UNIT_TARGET = /tmp/string_b3_unit

all: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS) $(HEADER)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(TEST_SRCS) -o $(TEST_TARGET)

unit: $(UNIT_TARGET)
	$(UNIT_TARGET)

$(UNIT_TARGET): $(SRC)/task1_unit/main_unit.cpp $(CORE) $(HEADER)
	$(CXX) $(CXXFLAGS) $^ -o $@

check: $(TEST_TARGET)
	./$(TEST_TARGET)

ASAN_TEST = /tmp/string_b3_asan_test
ASAN_UNIT = /tmp/string_b3_asan_unit
asan:
	$(CXX) $(CXXFLAGS) -fsanitize=address,leak $(TEST_SRCS) -o $(ASAN_TEST)
	$(CXX) $(CXXFLAGS) -fsanitize=address,leak $(SRC)/task1_unit/main_unit.cpp $(CORE) \
		-o $(ASAN_UNIT)
	$(ASAN_UNIT)
	$(ASAN_TEST)

clean:
	rm -f $(TEST_TARGET) $(UNIT_TARGET) $(ASAN_TEST) $(ASAN_UNIT)

.PHONY: all unit check asan clean