# Baseline 任务 3（栈和队列的综合应用）提交构建文件
#
# 一条命令完成编译：make     生成 test
# 运行样例比对：        make check
# 独立 oracle 校验：    make oracle   （Python 独立实现，非自洽）
# 清理构建产物：        make clean

CXX      = g++
# -flto：跨 TU 内联，保证性能数据可比
CXXFLAGS = -std=c++17 -Wall -O2 -flto

SRC = src

TARGETS = test

all: $(TARGETS)

# 队列实现栈 + 栈实现队列 + vector 互转 + 三方性能统计（规范的测试程序）
test: $(SRC)/main_test.cpp $(SRC)/stack_by_queue.cpp $(SRC)/queue_by_stack.cpp $(SRC)/vector_containers.cpp
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $@

check: test
	./test

# 独立 oracle：用 Python 独立实现（与 C++ 零共享代码）按 README 格式重算期望输出，
# 与 outputs/ 逐用例比对，打破"源码产出 → 源码比对"的自洽循环
oracle:
	python3 tools/oracle.py

clean:
	rm -f $(TARGETS)

.PHONY: all check oracle clean
# 头文件/模板实现改动后重新构建，避免旧二进制掩盖修复。
HEADERS := $(shell find src -type f \( -name "*.hpp" -o -name "*.tpp" \))
HEADERS += $(shell find ../src -name "*.hpp")
$(TARGETS): $(HEADERS)
