# Baseline 任务 2（队列的基本实现）提交构建文件
#
# 一条命令完成编译：make           生成 test / queue_methods / queue_extend
# 运行样例比对：        make check
# 两种解法对照：        make methods
# 扩展功能自测：        make extend
# 清理构建产物：        make clean
#
# 复用给定 queue.hpp 的实现（../src/queue/queue.cpp），不复制副本。

CXX      = g++
# -flto：跨 TU 内联，使手写队列与 std::queue 基准在同等条件下对比
CXXFLAGS = -std=c++17 -Wall -O2 -flto

SRC = src
QUE = ../src/queue

TARGETS = test queue_methods queue_extend

all: $(TARGETS)

# 小任务 1/3：循环队列 vs 链式队列交叉校验 + 三行性能统计（规范的测试程序）
test: $(SRC)/main_test.cpp $(SRC)/linked_queue.cpp $(QUE)/queue.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

# 小任务 2：两种 front/rear 解法对照
queue_methods: $(SRC)/main_methods.cpp $(SRC)/sized_queue.cpp $(QUE)/queue.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

# 小任务 4：扩展队列五功能自测 + 与 std::queue 对比
queue_extend: $(SRC)/main_extend.cpp $(SRC)/extended_queue.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

check: test
	./test

methods: queue_methods
	./queue_methods

extend: queue_extend
	./queue_extend

clean:
	rm -f $(TARGETS)

.PHONY: all check methods extend clean