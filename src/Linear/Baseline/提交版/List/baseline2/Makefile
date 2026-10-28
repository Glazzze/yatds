CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

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
# 注：Vector<T> 为 header-only（实现全在 vector.hpp，便于内联），故不编译 vector.cpp
test: $(T1)/main_basic.cpp $(LINKED)/linked_list.cpp
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $@

# 小任务 2：复用小任务 1 的实现，评估与 std::vector 的性能差异
vector_perf: $(T2)/main_performance.cpp
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $@

# 小任务 3：扩展版可变数组 + 功能自测与 std::vector 对比
vector_extend: $(T3)/main_extend.cpp
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $@

# 小任务 4：可变数组与链表的性能对比
array_list_cmp: $(T4)/main_compare.cpp $(LINKED)/linked_list.cpp
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $@

# 运行样例比对：自动遍历 inputs/ 与 outputs/（需在含样例目录的本目录执行）
check: test
	./$(word 1,$(TARGETS))

# 编译并运行小任务 3 的扩展功能自测
unit: vector_extend regression
	./vector_extend

# 完整性能分析（多规模、多场景）：与 test 的功能比对解耦，单独运行
bench: vector_perf array_list_cmp
	./vector_perf
	./array_list_cmp

# 内存检查：用 ASan/LSan 重编“分配/释放堆内存”的两个功能程序并运行，验证零泄漏 / 无未定义行为
# （test 在本目录执行以读到 inputs/；vector_extend 输出到 /tmp，避免污染提交目录；
#   性能基准程序 array_list_cmp 复用同一套容器代码，规模大、不适合在 ASan 下运行）
ASAN_TEST = /tmp/b2_asan_test
ASAN_EXT  = /tmp/b2_asan_ext
asan:
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined $(T1)/main_basic.cpp $(LINKED)/linked_list.cpp -o $(ASAN_TEST)
	$(CXX) $(CXXFLAGS) -fsanitize=address,undefined $(T3)/main_extend.cpp -o $(ASAN_EXT)
	$(ASAN_TEST)
	$(ASAN_EXT)

clean:
	rm -f $(TARGETS) $(ASAN_TEST) $(ASAN_EXT)

.PHONY: all check unit bench asan clean
# 头文件/模板实现改动后重新构建，避免旧二进制掩盖修复。
HEADERS := $(shell find src -type f \( -name "*.hpp" -o -name "*.tpp" \))
$(TARGETS): $(HEADERS)

ALIAS_BASIC = /tmp/vector_alias_basic
ALIAS_EXT = /tmp/vector_alias_extended
regression:
	$(CXX) $(CXXFLAGS) src/tests/test_vector_alias.cpp -o $(ALIAS_BASIC)
	$(ALIAS_BASIC)
	$(CXX) $(CXXFLAGS) -DVECTOR_EXTENDED src/tests/test_vector_alias.cpp -o $(ALIAS_EXT)
	$(ALIAS_EXT)
asan-regression:
	$(CXX) $(CXXFLAGS) -g -fsanitize=address,undefined src/tests/test_vector_alias.cpp -o $(ALIAS_BASIC)
	$(ALIAS_BASIC)
	$(CXX) $(CXXFLAGS) -g -DVECTOR_EXTENDED -fsanitize=address,undefined src/tests/test_vector_alias.cpp -o $(ALIAS_EXT)
	$(ALIAS_EXT)
asan: asan-regression
.PHONY: regression asan-regression

statistics-regression:
	$(CXX) $(CXXFLAGS) src/tests/test_statistics.cpp -o /tmp/list_statistics_regression
	/tmp/list_statistics_regression
regression: statistics-regression
.PHONY: statistics-regression
