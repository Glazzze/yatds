# 栈与队列（StackAndQueue）

## 提交材料导航与运行说明

本文件汇总三个 Baseline 的材料位置、运行方式、提交要求对照和验证结果。详细设计、结果分析、人机协作过程及截图见 [实验报告（PDF）](实验报告.pdf)。

### 材料位置

| 材料 | 位置 | 说明 |
| --- | --- | --- |
| 实验报告 | [实验报告.pdf](实验报告.pdf) | 项目分析、协作过程、运行截图与总结 |
| 顶层复现入口 | [Makefile](Makefile) | 样例比对、独立 oracle、内存检查、性能统计与空间测量 |
| 公共依赖源码 | [src/](src/) | 数组栈、链式栈与循环队列的基础实现及给定测试程序 |
| 栈的基本实现 | [baseline1/](baseline1/) | 基础栈交叉校验、扩展栈与性能对比 |
| 队列的基本实现 | [baseline2/](baseline2/) | 循环与链式队列、两种回绕方案、扩展功能 |
| 栈与队列的综合应用 | [baseline3/](baseline3/) | 用队列实现栈、用栈实现队列、vector 实现与双向转换 |
| 性能与空间工具 | [tools/](tools/) | 多次运行统计、Welch t 检验与 RSS 空间探针 |

### 编译和运行

在本 README 所在目录执行，需要支持 C++17 的编译器、`make` 与 Python 3。各 Baseline 的默认编译参数为 `-std=c++17 -Wall -O2 -flto`。

```sh
make -C baseline1 all  # 编译 test 和 stack_extend
make -C baseline2 all  # 编译 test、queue_methods 和 queue_extend
make -C baseline3 all  # 编译 test

make check             # 三个 Baseline 自动读取 inputs/ 并比对 outputs/
make oracle            # Python 独立重算期望输出，校验标准输出样例
make -C baseline1 extend  # 扩展栈功能自测
make -C baseline2 methods # 两种队列回绕方案对照
make -C baseline2 extend  # 扩展队列功能自测

make asan              # 三个 test 的 AddressSanitizer/UBSan 检查
make perf              # 运行 30 次统计均值、标准差与 Welch t 检验
make space             # 数组、链表和标准容器的当前 RSS 空间探针
```

顶层直接运行 `make` 或 `make all` 会依次执行样例比对、独立 oracle 和内存检查；它与 List 顶层仅编译的行为不同。`make perf` 自动构建所需程序；`make unit` 汇总扩展功能与回绕方案自测。

**公共依赖必须随包保留**：baseline1 通过相对路径引用 `src/array_stack/` 与 `src/linked_stack/`，baseline2 引用 `src/queue/`。单独复制 baseline1 或 baseline2 目录无法完成编译；应保持当前 `StackAndQueue/` 整体结构。

### Baseline 概览

| Baseline | 主题 | 源码 | 输入与标准输出 | 样例结果 | 其他功能验证 |
| --- | --- | --- | --- | --- | --- |
| baseline1 | 栈的基本实现，三个小任务 | [src/](baseline1/src/) + 公共栈实现 | [inputs/](baseline1/inputs/) / [outputs/](baseline1/outputs/) | 8/8 PASS | oracle 8/8；扩展栈 62 项断言 |
| baseline2 | 队列的基本实现，四个小任务 | [src/](baseline2/src/) + 公共队列实现 | [inputs/](baseline2/inputs/) / [outputs/](baseline2/outputs/) | 8/8 PASS | oracle 8/8；方案对照 4 项、扩展队列 33 项 |
| baseline3 | 栈与队列综合应用，五个小任务 | [src/](baseline3/src/) | [inputs/](baseline3/inputs/) / [outputs/](baseline3/outputs/) | 4/4 PASS | oracle 4/4；空结构、双向转换与交替操作样例 |

以上编译、样例、oracle 和功能自测已在提交版副本中复核通过。内存检查和多次运行性能统计沿用实验报告中的验证记录，可用上述命令复现；具体耗时随机器和编译器变化。

## 提交要求逐项对照

### 通则（三个 Baseline 共同适用）

| 提交要求 | 证据位置 | 结果或复核方式 |
| --- | --- | --- |
| 输入样例与同名标准输出 | 各 `baselineN/inputs/`、`baselineN/outputs/` | 共 20 组，同名配对齐全 |
| 名为 test 的自动比对程序 | 各 `baselineN/test` 与 `src/main_test.cpp` | 自动读取样例，逐例输出 PASS/FAIL |
| 完整实现源码与必要头文件 | 各 `baselineN/src/` 及顶层 [src/](src/) | 公共依赖已包含，可重新编译 |
| 一条命令编译 | 各 `baselineN/Makefile` | 进入对应目录运行 `make`；顶层运行方式见上文 |
| 独立校验标准输出 | 各 `baselineN/tools/oracle.py` | Python 独立实现，20/20 样例通过 |
| 程序、测试与性能分析 | [实验报告.pdf](实验报告.pdf) 和各 Baseline 的 README | 包含设计、边界、测试结果与性能对比 |
| 内存与未定义行为检查 | 顶层 [Makefile](Makefile) 的 asan 目标 | 报告记录三个 test 在 ASan/UBSan 下干净退出 |

### baseline1：栈的基本实现

| 任务内容 / 提交要求 | 证据 | 结果或说明 |
| --- | --- | --- |
| ① 数组栈与链式栈实现、测试 | [array_stack/](src/array_stack/)、[linked_stack/](src/linked_stack/)、[main_test.cpp](baseline1/src/main_test.cpp) | 给定接口的实现及测试程序随包保留；命令式测试重放同一操作序列，交叉校验两种实现，8/8 样例通过 |
| ② 两种栈的性能比较 | [main_test.cpp](baseline1/src/main_test.cpp)、[性能统计工具](tools/perf_stats.py) | 输出 array_stack、linked_stack、std_stack 的 PERF 数据；报告附多规模、30 次运行统计 |
| ③ 至少五个扩展功能及标准栈对比 | [extended_stack.hpp](baseline1/src/extended_stack.hpp)、[extended_stack.cpp](baseline1/src/extended_stack.cpp)、[main_extend.cpp](baseline1/src/main_extend.cpp) | clear、swap、search、peek、reverse；62 项断言通过，并输出与 std::stack 的性能比较 |
| 程序与结果分析 | [baseline1/README.md](baseline1/README.md) | 基础接口、容量边界、扩展功能、性能与复杂度说明 |

### baseline2：队列的基本实现

| 任务内容 / 提交要求 | 证据 | 结果或说明 |
| --- | --- | --- |
| ① 数组队列与链式队列实现、测试 | [queue/](src/queue/)、[linked_queue.cpp](baseline2/src/linked_queue.cpp)、[main_test.cpp](baseline2/src/main_test.cpp) | 循环队列与链式队列交叉校验，8/8 样例通过 |
| ② 至少两种 front/rear 到达末尾的解决方案 | [queue.cpp](src/queue/queue.cpp)、[sized_queue.cpp](baseline2/src/sized_queue.cpp)、[main_methods.cpp](baseline2/src/main_methods.cpp) | 环形空槽法与 size 计数法；方案对照 4 项通过 |
| ③ 两种队列的性能比较 | [main_test.cpp](baseline2/src/main_test.cpp)、[性能统计工具](tools/perf_stats.py) | 输出 circular_queue、linked_queue、std_queue 的 PERF 数据；另比较两种回绕方案 |
| ④ 至少五个扩展功能及标准队列对比 | [extended_queue.hpp](baseline2/src/extended_queue.hpp)、[extended_queue.cpp](baseline2/src/extended_queue.cpp)、[main_extend.cpp](baseline2/src/main_extend.cpp) | size、clear、swap、back、search；33 项断言通过，并输出与 std::queue 的性能比较 |
| 方案说明与结果分析 | [baseline2/README.md](baseline2/README.md) | 两种回绕方案、空满边界、容量利用率与性能分析 |

### baseline3：栈和队列的综合应用

| 任务内容 / 提交要求 | 证据 | 结果或说明 |
| --- | --- | --- |
| ① 使用队列实现栈 | [stack_by_queue.hpp](baseline3/src/stack_by_queue.hpp)、[stack_by_queue.cpp](baseline3/src/stack_by_queue.cpp) | 单队列旋转法，支持入栈、出栈和访问栈顶 |
| ② 使用栈实现队列 | [queue_by_stack.hpp](baseline3/src/queue_by_stack.hpp)、[queue_by_stack.cpp](baseline3/src/queue_by_stack.cpp) | 双栈法，支持入队、出队和访问队首 |
| ③ vector 实现栈、队列与双向转换 | [vector_containers.hpp](baseline3/src/vector_containers.hpp)、[vector_containers.cpp](baseline3/src/vector_containers.cpp) | 支持 STACK_TO_QUEUE 与 QUEUE_TO_STACK，样例覆盖两种转换 |
| ④ 栈实现队列与 vector 队列的比较 | [main_test.cpp](baseline3/src/main_test.cpp) | queue_by_stack 与 vector_based 同负载性能统计 |
| ⑤ 队列实现栈与 vector 栈的比较 | [main_test.cpp](baseline3/src/main_test.cpp) | stack_by_queue 与 vector_stack 同负载性能统计 |
| 完整测试与原理分析 | [baseline3/README.md](baseline3/README.md)、[实验报告.pdf](实验报告.pdf) | 4/4 样例与独立 oracle 通过；说明旋转法、双栈法及复杂度 |

## 报告截图索引

以下截图已嵌入 [实验报告.pdf](实验报告.pdf)，按报告中的图号定位。

| 报告图号 | 内容 |
| --- | --- |
| 图 1 | 确认公共依赖源码用途与目录架构 |
| 图 2 | baseline1 的代码风格、注释与小任务划分要求 |
| 图 3 | 将基础实现迁移合并到给定骨架，保留头文件接口 |
| 图 4 | 整体核对三个 Baseline 的任务与提交要求 |
| 图 5 | 单独核对 baseline3 的输入输出与样例 |
| 图 6 | 队列实现栈：单队列旋转与双队列法的取舍 |
| 图 7 | 独立 oracle 定位并修复 case06 格式问题 |
| 图 8 | 三个 test 的 ASan/UBSan 检查结果 |
| 图 9 | 三个 Baseline 的样例比对与完整 PERF 输出（三张截图） |
| 图 10 | 多次运行统计、Welch t 检验与 RSS 空间测量（三张截图） |
| 图 11 | 扩展栈 62 项断言与性能对比 |
| 图 12 | 两种队列回绕方案 4 项、扩展队列 33 项自测 |
| 图 13 | baseline3 四场景运行结果，含双向转换 |

## 已知边界与不足

- `VectorQueue` 出队后保留历史存储，长期反复入队、出队时空间占用会增长。
