CXX      = g++
CXXFLAGS = -std=c++17 -Wall

SRC      = src
LIST     = $(SRC)/common/basic_list/list.cpp
ALGO     = $(SRC)/task1_algorithms/ghost_intersection.cpp

# 两个可执行程序：命令式测试（test）与手动构造可视化测试（manual）
TARGETS  = test manual

all: $(TARGETS)

# 命令式测试程序：读取 inputs/ 样例并与 outputs/ 比对（命名为 test，满足提交规范）
test: $(SRC)/task3_command_test/main_case.cpp $(ALGO) $(LIST)
	$(CXX) $(CXXFLAGS) $^ -o $@

# 手动构造 + 可视化验证程序
manual: $(SRC)/task2_manual_test/main_manual.cpp $(ALGO) $(LIST)
	$(CXX) $(CXXFLAGS) $^ -o $@

# 运行样例比对（test 已被二进制占用，故运行目标名为 check）
check: test
	./test

# 运行手动构造测试
unit: manual
	./manual

# 内存检查：用 ASan/LSan 重编两个程序并运行，验证零泄漏 / 无双重释放
# （在本目录执行，故 test 程序能读到 inputs/）
asan:
	$(CXX) $(CXXFLAGS) -fsanitize=address,leak $(SRC)/task3_command_test/main_case.cpp $(ALGO) $(LIST) -o /tmp/b3_test
	$(CXX) $(CXXFLAGS) -fsanitize=address,leak $(SRC)/task2_manual_test/main_manual.cpp $(ALGO) $(LIST) -o /tmp/b3_manual
	/tmp/b3_test
	/tmp/b3_manual

clean:
	rm -f $(TARGETS) /tmp/b3_test /tmp/b3_manual

.PHONY: all check unit asan clean