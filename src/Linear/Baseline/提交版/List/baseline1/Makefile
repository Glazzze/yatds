CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

SRC      = src

# 命令式测试程序（小任务 3）：可执行文件名为 test（按提交规范）
# 程序自身会自动遍历 inputs/ 与 outputs/ 并逐用例比对
TEST_SRCS   = $(SRC)/task3_command_test/test_command.cpp \
              $(SRC)/task2_linked_list/linked_list.cpp
TEST_TARGET = test

# 单元验证程序（小任务 1、2）输出到 /tmp，避免污染提交目录
UNIT1 = /tmp/test_basic_list
UNIT2 = /tmp/test_linked_list

all: $(TEST_TARGET)

# 注意：目标名 test 已被可执行文件占用，不可再定义同名伪目标；
# 运行样例比对请使用 make check
$(TEST_TARGET): $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $(TEST_TARGET)

# 编译并运行小任务 1、2 的单元验证程序
unit: unit1 unit2 generic

unit1: $(SRC)/task1_basic_list/list.cpp $(SRC)/task1_basic_list/test_list.cpp
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $(UNIT1)
	$(UNIT1)

unit2: $(SRC)/task2_linked_list/linked_list.cpp $(SRC)/task2_linked_list/test_linked_list.cpp
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $(UNIT2)
	$(UNIT2)

# 运行样例比对：自动遍历 inputs/ 与 outputs/（需在含样例目录的本目录执行）
check: $(TEST_TARGET)
	./$(TEST_TARGET)

# 内存检查：用 ASan/LSan 重编三个程序并运行，验证零泄漏 / 无未定义行为
# （在本目录执行，故 test 能读到 inputs/；单元程序输出到 /tmp 避免污染提交目录）
ASAN1 = /tmp/b1_asan_basic
ASAN2 = /tmp/b1_asan_linked
ASAN3 = /tmp/b1_asan_test
asan:
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined $(SRC)/task1_basic_list/list.cpp $(SRC)/task1_basic_list/test_list.cpp -o $(ASAN1)
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined $(SRC)/task2_linked_list/linked_list.cpp $(SRC)/task2_linked_list/test_linked_list.cpp -o $(ASAN2)
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined $(TEST_SRCS) -o $(ASAN3)
	$(ASAN1)
	$(ASAN2)
	$(ASAN3)

clean:
	rm -f $(TEST_TARGET) $(UNIT1) $(UNIT2) $(ASAN1) $(ASAN2) $(ASAN3)

.PHONY: all unit unit1 unit2 check asan clean

# 头文件/模板实现改动后重新构建，避免旧二进制掩盖修复。
HEADERS := $(shell find src -type f \( -name "*.hpp" -o -name "*.tpp" \))
$(TEST_TARGET): $(HEADERS)

GENERIC = /tmp/list_generic_regression
generic:
	$(CXX) $(CXXFLAGS) $(SRC)/task2_linked_list/test_generic_types.cpp $(SRC)/task2_linked_list/linked_list.cpp -o $(GENERIC)
	$(GENERIC)
asan-generic:
	$(CXX) $(CXXFLAGS) -g -fsanitize=address,undefined $(SRC)/task2_linked_list/test_generic_types.cpp -o $(GENERIC)
	$(GENERIC)
asan: asan-generic
.PHONY: generic asan-generic
