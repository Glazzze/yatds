# 栈和队列 · Baseline 任务 3（栈和队列的综合应用）提交说明

本目录（`baseline3/`）完成课程 README 中 Baseline 任务 3。课程 [README](../README.md) 为原始任务文档，不做改动。

## 目录结构

```text
StackAndQueue/
├── README.md              ← 课程原始任务文档（未改动）
└── baseline3/             ← 本提交目录（Baseline 任务 3）
    ├── Makefile           ← make 生成 test
    ├── README.md          ← 本提交说明
    ├── src/
    │   ├── stack_by_queue.hpp/.cpp    ← 队列实现栈
    │   ├── queue_by_stack.hpp/.cpp    ← 栈实现队列
    │   ├── vector_containers.hpp/.cpp ← <vector> 实现的栈与队列 + 互转
    │   └── main_test.cpp              ← T 场景驱动（编译为 test）
    ├── inputs/            ← 自测样例（case01 ~ case04）
    └── outputs/
```

## 小任务划分

| 小任务 | 文件 | 内容 |
| --- | --- | --- |
| 1 | `stack_by_queue.*` | 用队列实现栈（栈基本操作） |
| 2 | `queue_by_stack.*` | 用栈实现队列（队列基本操作） |
| 3 | `vector_containers.*` | `<vector>` 实现栈与队列，各带互转函数 |
| 4/5 | `main_test.cpp`（PERF 部分） | 两组性能对比 |

## 构建与自测

```sh
make           # 生成 test
make check     # 样例比对
```

## 小任务 1/2：实现原理

**用队列实现栈（`StackByQueue`）——单队列旋转法**
- 入栈：直接加入队尾，O(1)。
- 栈顶：最后入队者即队尾（`std::queue::back()`），O(1)。
- 出栈：需移除队尾元素，但队列只能从队首出队。做法是**旋转**——把队首元素逐个移到队尾 n-1 次，使原队尾（栈顶）来到队首，再出队。O(n)。
- 只用一个队列即可（双队列法亦可，但单队列旋转更简洁）。

**用栈实现队列（`QueueByStack`）——双栈法**
- 入队：压入 `in_` 栈，O(1)。
- 出队/队首：维护两个栈，`in_` 负责入队、`out_` 负责出队。当 `out_` 为空时，把 `in_` 的元素全部倒入 `out_`——顺序被连续反转两次，最早入队的元素来到 `out_` 栈顶。此后出队即 `out_.pop()`。
- 每个元素至多被移动一次，**均摊 O(1)**。

**用 `<vector>` 实现（`VectorStack` / `VectorQueue`）与互转**
- `VectorStack`：`vector` 尾部为栈顶。
- `VectorQueue`：`vector` + 队首下标 `head_`（出队即 `head_++`，避免头部删除的 O(n)）。
- 互转：`VectorStack::to_queue()` 按出栈顺序入队（栈顶→队首）；`VectorQueue::to_stack()` 按出队顺序入栈（队首→栈底）。两者在同一头文件中定义，避免循环依赖。

## 程序与结果分析

`test` 读入 `T` 个场景，逐场景先输出 `CASE t`，支持 `STACK_BY_QUEUE`、`QUEUE_BY_STACK`、`CONVERT`（`STACK_TO_QUEUE`/`QUEUE_TO_STACK`）三类；空结构访问输出 `ERROR`、空打印输出 `EMPTY`。因各结构的接口只暴露栈顶/队首操作，`PRINT` 用倒栈/倒队技巧遍历后恢复。

**结果**：`make check` case01~case04 全部 PASS，其中 case01 即父级 README 的三场景样例，输出逐字一致；case02 覆盖空结构边界；case03 覆盖双向转换；case04 覆盖交替操作。

## 小任务 4/5：两组性能对比

（每项 3 次取平均，`-O2 -flto`；`stack_by_queue` 的 pop 为 O(n)，规模取小值）

| 实现 | n=1000 | n=5000 | 说明 |
| --- | --- | --- | --- |
| `stack_by_queue`（队列实现栈） | 1.76 ms | 30.32 ms | push O(1)、pop O(n) |
| `vector_stack`（vector 实现栈） | 0.00 ms | 0.02 ms | push/pop 均摊 O(1) |
| `queue_by_stack`（栈实现队列） | 0.01 ms | 0.04 ms | 双栈均摊 O(1) |
| `vector_based`（vector 实现队列） | 0.01 ms | 0.03 ms | push/pop 均摊 O(1) |

**对比一：队列实现栈 vs vector 实现栈**。`stack_by_queue` 慢 3 个数量级以上，且随 n 增长呈二次方恶化（n 从 1000 到 5000，时间从 1.76ms 涨到 30.32ms，约 17 倍）。根本原因是**出栈必须旋转整个队列**（O(n)），n 次出栈合计 O(n²)；而 `VectorStack` 的 push/pop 都是均摊 O(1)。这体现了"用受限结构（只有队首操作）模拟栈"的固有代价：栈关心的是**最近**元素，而队列只能从**最早**一端取出，二者语义正交，转换必然引入额外搬运。

**对比二：栈实现队列 vs vector 实现队列**。`queue_by_stack` 与 `vector_based` 同数量级（0.04ms vs 0.03ms），双栈法的均摊 O(1) 使其几乎不逊于直接实现。原因是"用两个栈模拟队列"只是把元素在 `in_`/`out_` 之间搬运一次（摊还），没有栈↔队列语义上的反复冲突，故开销被均摊掉。这解释了为何**库设计者常用双栈/双队列做适配**，而反向（队列适配栈）则代价高昂。

**结论**：栈与队列可以相互模拟，但代价不对称——用两个同向结构（栈→队列）均摊后可接近原生性能；用单结构改向（队列→栈）则因出栈需整体旋转而不划算。生产代码应直接选用语义匹配的容器；本实验的价值在于量化这一权衡。

## 验证记录

- `make`：编译通过，`-Wall` 无警告。
- `make check`：case01~case04 全部 PASS（case01 与父级 README 样例逐字一致）。
- 性能数据取 `-O2 -flto`、每项 3 次取平均的单次运行实测值。