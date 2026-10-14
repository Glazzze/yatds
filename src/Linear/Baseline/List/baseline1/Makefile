CXX      = g++
CXXFLAGS = -std=c++17 -Wall

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
unit: unit1 unit2

unit1: $(SRC)/task1_basic_list/list.cpp $(SRC)/task1_basic_list/test_list.cpp
	$(CXX) $(CXXFLAGS) $^ -o $(UNIT1)
	$(UNIT1)

unit2: $(SRC)/task2_linked_list/linked_list.cpp $(SRC)/task2_linked_list/test_linked_list.cpp
	$(CXX) $(CXXFLAGS) $^ -o $(UNIT2)
	$(UNIT2)

# 运行样例比对：自动遍历 inputs/ 与 outputs/（需在含样例目录的本目录执行）
check: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(TEST_TARGET) $(UNIT1) $(UNIT2)

.PHONY: all unit unit1 unit2 check clean
