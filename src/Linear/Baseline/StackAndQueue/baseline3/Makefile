# Baseline 任务 3（栈和队列的综合应用）提交构建文件
#
# 一条命令完成编译：make     生成 test
# 运行样例比对：        make check
# 清理构建产物：        make clean

CXX      = g++
# -flto：跨 TU 内联，保证性能数据可比
CXXFLAGS = -std=c++17 -Wall -O2 -flto

SRC = src

TARGETS = test

all: $(TARGETS)

# 队列实现栈 + 栈实现队列 + vector 互转 + 三方性能统计（规范的测试程序）
test: $(SRC)/main_test.cpp $(SRC)/stack_by_queue.cpp $(SRC)/queue_by_stack.cpp $(SRC)/vector_containers.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

check: test
	./test

clean:
	rm -f $(TARGETS)

.PHONY: all check clean