CXX      = g++
CXXFLAGS = -std=c++17 -Wall -O2

SRC      = src
T1       = $(SRC)/task1_vector
T2       = $(SRC)/task2_performance
T3       = $(SRC)/task3_extended_vector
T4       = $(SRC)/task4_array_vs_list
LINKED   = $(SRC)/common/linked_list

# 四个可执行程序：命令式测试 / 与 std::vector 性能对比 / 扩展功能自测 / 数组-链表对比
TARGETS  = test vector_perf vector_extend array_list_cmp

all: $(TARGETS)

# 小任务 1：可变数组功能测试程序（读取 inputs/ 样例格式，命名为 test）
test: $(T1)/main_basic.cpp $(T1)/vector.cpp $(LINKED)/linked_list.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

# 小任务 2：复用小任务 1 的实现，评估与 std::vector 的性能差异
vector_perf: $(T2)/main_performance.cpp $(T1)/vector.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

# 小任务 3：扩展版可变数组 + 功能自测与 std::vector 对比
vector_extend: $(T3)/main_extend.cpp $(T3)/vector.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

# 小任务 4：可变数组与链表的性能对比
array_list_cmp: $(T4)/main_compare.cpp $(T1)/vector.cpp $(LINKED)/linked_list.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

# 运行样例比对：自动遍历 inputs/ 与 outputs/（需在含样例目录的本目录执行）
check: test
	./$(word 1,$(TARGETS))

# 编译并运行小任务 3 的扩展功能自测
unit: vector_extend
	./vector_extend

clean:
	rm -f $(TARGETS)

.PHONY: all check unit clean