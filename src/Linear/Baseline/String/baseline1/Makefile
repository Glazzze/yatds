CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
OPTFLAGS = -O2

SRC      = src

# 命令式测试程序（提交名 test）：自动遍历 inputs/ 与 outputs/ 逐用例比对
TEST_SRCS   = $(SRC)/task4_command_test/main_test.cpp \
              $(SRC)/my_string/mystring.cpp
TEST_TARGET = test

# 单元验证与性能实验程序输出到 /tmp，避免污染提交目录
UNIT_TARGET = /tmp/string_b1_unit
BENCH_TARGET = /tmp/string_b1_bench

all: $(TEST_TARGET)

# 目标名 test 已被可执行文件占用，运行样例比对请使用 make check
$(TEST_TARGET): $(TEST_SRCS) $(SRC)/my_string/mystring.hpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(TEST_SRCS) -o $(TEST_TARGET)

unit: $(UNIT_TARGET)
	$(UNIT_TARGET)

$(UNIT_TARGET): $(SRC)/task2_extended/main_extended.cpp $(SRC)/my_string/mystring.cpp \
                $(SRC)/my_string/mystring.hpp
	$(CXX) $(CXXFLAGS) $^ -o $@

bench: $(BENCH_TARGET)
	$(BENCH_TARGET)

$(BENCH_TARGET): $(SRC)/task3_performance/main_performance.cpp $(SRC)/my_string/mystring.cpp \
                 $(SRC)/my_string/mystring.hpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

check: $(TEST_TARGET)
	./$(TEST_TARGET)

# 内存检查：ASan/LSan 下重编命令式程序与单元程序并运行，验证零泄漏 / 无未定义行为
ASAN_TEST  = /tmp/string_b1_asan_test
ASAN_UNIT  = /tmp/string_b1_asan_unit
asan:
	$(CXX) $(CXXFLAGS) -fsanitize=address,leak $(TEST_SRCS) -o $(ASAN_TEST)
	$(CXX) $(CXXFLAGS) -fsanitize=address,leak $(SRC)/task2_extended/main_extended.cpp \
		$(SRC)/my_string/mystring.cpp -o $(ASAN_UNIT)
	$(ASAN_UNIT)
	$(ASAN_TEST)

clean:
	rm -f $(TEST_TARGET) $(UNIT_TARGET) $(BENCH_TARGET) $(ASAN_TEST) $(ASAN_UNIT)

.PHONY: all unit bench check asan clean