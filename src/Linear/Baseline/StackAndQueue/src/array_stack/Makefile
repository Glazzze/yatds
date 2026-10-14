# 数组栈（Baseline 任务 1 · 数组栈组）构建文件
#
# 一条命令完成编译：make      （生成 test）
# 运行样例比对：        make check
# 清理构建产物：        make clean
#
# 注：使用 -O2 优化以保证 PERF 性能统计数据有参考意义；功能行为不受影响。
# 可执行文件名为 test（按提交规范），因此运行样例的目标名为 check，
# 避免与二进制同名冲突导致 make 循环依赖。

CXX      = g++
# -flto：跨 TU 内联，使手写栈的 push/pop 与 std 基准在同等条件下对比
CXXFLAGS = -std=c++17 -Wall -O2 -flto

TARGET = test

.PHONY: all check clean

all: $(TARGET)

# 数组栈实现 + 测试程序（接口自检 + 命令式比对 + 性能统计）
$(TARGET): stackar.cpp stackar.hpp teststka.cpp
	$(CXX) $(CXXFLAGS) stackar.cpp teststka.cpp -o $(TARGET)

# 运行样例比对：程序自动遍历 inputs/ 与 outputs/（需在本目录执行）
check: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
