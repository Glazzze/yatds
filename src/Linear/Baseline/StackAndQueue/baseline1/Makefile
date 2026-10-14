# Baseline 任务 1（栈的基本实现）提交构建文件
#
# 一条命令完成编译：make          生成 test（命令式测试）与 stack_extend（扩展自测）
# 运行样例比对：        make check
# 运行扩展功能自测：    make extend
# 清理构建产物：        make clean
#
# 复用的依赖代码（给定骨架的实现）位于 ../src/array_stack 与 ../src/linked_stack，
# 通过相对路径引用，不复制副本。

CXX      = g++
# -flto：跨 TU 内联，使手写栈与 std::stack 基准在同等条件下对比
CXXFLAGS = -std=c++17 -Wall -O2 -flto

SRC = src
ARR = ../src/array_stack
LNK = ../src/linked_stack

TARGETS = test stack_extend

all: $(TARGETS)

# 小任务 1/2：数组栈 vs 链表栈交叉校验 + 三行性能统计（规范的测试程序）
test: $(SRC)/main_test.cpp $(ARR)/stackar.cpp $(LNK)/stackli.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

# 小任务 3：扩展栈五功能自测 + 与 std::stack 性能对比
stack_extend: $(SRC)/main_extend.cpp $(SRC)/extended_stack.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

# 运行样例比对（程序自动遍历 inputs/ 与 outputs/）
check: test
	./test

extend: stack_extend
	./stack_extend

clean:
	rm -f $(TARGETS)

.PHONY: all check extend clean