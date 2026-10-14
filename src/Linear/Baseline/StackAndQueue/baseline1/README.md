# 栈和队列 · Baseline 任务 1（栈的基本实现）提交说明

本目录（`baseline1/`）完成课程 README 中 Baseline 任务 1 的三个小任务。课程 [README](../README.md) 为原始任务文档，不做改动；本文件合并目录结构、构建方式、源码组织与结果分析，作为提交材料的完整说明。

## 目录结构

```text
StackAndQueue/
├── README.md              ← 课程原始任务文档（未改动）
├── src/                   ← 依赖代码（给定骨架的实现，见下）
│   ├── array_stack/       ← stackar.hpp(给定) + stackar.cpp(实现)
│   ├── linked_stack/      ← stackli.hpp(给定) + stackli.cpp(实现)
│   └── queue/
└── baseline1/             ← 本提交目录（Baseline 任务 1）
    ├── Makefile           ← make 生成 test / stack_extend；make check 运行样例比对
    ├── README.md          ← 本提交说明
    ├── src/
    │   ├── main_test.cpp        ← 小任务 1/2：两实现交叉校验 + 三行性能统计（编译为 test）
    │   ├── extended_stack.hpp/.cpp ← 小任务 3：扩展栈五功能
    │   └── main_extend.cpp      ← 小任务 3：扩展自测 + 与 std::stack 对比（编译为 stack_extend）
    ├── inputs/            ← 自测样例（case01 ~ case08）
    └── outputs/           ← 与 inputs/ 同名的标准输出
```

依赖代码复用 `../src/array_stack` 与 `../src/linked_stack` 中的实现（相对路径引用，不复制副本，避免与给定骨架漂移）。

## 小任务划分

| 小任务 | 文件 | 可执行程序 | 内容 |
| --- | --- | --- | --- |
| 1 | `src/main_test.cpp` | `test` | 命令式测试：同一序列在数组栈与链表栈上重放并**逐行交叉校验**，读 `inputs/`、比对 `outputs/` |
| 2 | `src/main_test.cpp`（PERF 部分） | `test` | 数组栈 / 链表栈 / `std::stack` 三方性能对比 |
| 3 | `src/extended_stack.*`、`src/main_extend.cpp` | `stack_extend` | 参照 `<stack>` 扩展 5 个功能 + 自测 + 与 `std::stack` 性能对比 |

## 构建与自测

```sh
make           # 生成 test 与 stack_extend
make check     # 运行样例比对：case01: PASS / ...
make extend    # 编译并运行扩展功能自测
make clean     # 清理构建产物
```

## 小任务 1：程序与结果分析

**实现要点**（给定骨架 `dsaac::ArrayStack<T>` / `LinkedStack<T>`）：

- **数组栈**：以 `std::vector` 尾部为栈顶，`push`/`pop` 即 `push_back`/`pop_back`。**固定容量语义**——构造时 `reserve(capacity)`，`full()` 判断 `size() >= capacity()`，满栈 `push` 抛 `std::overflow_error`，空栈 `pop`/`top` 抛 `std::out_of_range`。因接口无 `size()`，`full()` 用 `vector::capacity()` 作容量上界（`push` 前先判满，`vector` 永不扩容，故 `capacity()` 恒等于构造容量）。
- **链表栈**：以 `std::forward_list` 头部为栈顶，`push_front`/`pop_front`。链表按需分配节点，**无容量上限**，接口不含 `full()`。
- **交叉校验**：`test` 把同一初始元素与操作序列在两种实现上重放，逐行比对；任一实现输出不一致都会在 stderr 报「内部错误」。同一份样例同时验证两种实现，比分别测试更强。为使两实现输出可比，**驱动层统一以输入的 `capacity` 为上界对两栈限流**（链式栈的"无界"仅体现在类本身）。
- **受限接口下的 SIZE / PRINT**：两者接口均无 `size()`、无遍历。驱动用**倒栈技巧**——逐个弹出到 `vector`（天然得到栈顶→栈底序），计数/打印后逆序压回恢复原状，O(n)。

**结果**：`make check` case01~case08 全部 PASS；数组栈与链表栈在全部样例上输出完全一致，印证"栈可视为受限制的数组或链表"。

样例覆盖：正常操作（case01）、空栈（case02/04）、**满栈边界**（case03 越界 PUSH 输出 ERROR；case08 恰好装满后弹出再入栈）、弹出后复用（case04/07）、负数重复元素（case05）、中间态（case06）。

## 小任务 2：性能差异分析

`test` 末尾输出三行 PERF（每项重复 3 次取平均，`-O2 -flto`，与 `std::stack` 同负载）：

| 实现 | n=100000 | n=1000000 |
| --- | --- | --- |
| `array_stack` | 0.66 ms | 4.80 ms |
| `linked_stack` | 4.17 ms | 29.46 ms |
| `std_stack` | 0.17 ms | 1.15 ms |

**分析**：

1. **连续内存 vs 分散节点（最主导因素）**。数组栈元素连续存放，`push` 只写一个 `int` 并更新长度，缓存行天然覆盖后续写入，预取器持续命中；链表栈每次 `push` 都要 `new` 一个节点（数据 + `next` 指针 + 堆元数据），节点散布在堆的不同位置，缓存局部性差，分配/释放本身也有开销。n=1000000 时数组栈快约 6 倍。
2. **空间开销**。链表每个元素额外携带指针与堆分配头部，存储 100 万个 `int` 的内存占用约为数组的数倍，进一步加剧缓存缺失与 TLB 压力。
3. **`std::stack` 为何仍最快**。它是**容器适配器**，默认底层 `std::deque`——分段连续存储，入栈只写块尾、块满时只追加一个新块指针，**从不整体搬移**既有元素。本实验的数组栈构造时已 `reserve`（等价于一次性分配），无 `vector` 扩容搬移，因此与 `std::stack` 的差距主要来自适配器的零开销封装与 `deque` 的分段布局。
4. **结论**：吞吐优先用数组栈（缓存友好、无逐节点分配）；单次操作延迟须可预测、或元素体积大/需节点地址稳定时用链表栈（严格 O(1)、无整体搬移）。生产代码可直接用 `std::stack`；自实现的价值在于理解这些权衡。

## 小任务 3：扩展功能与 `<stack>` 对比

因给定 `stackar.hpp` / `stackli.hpp` **不可修改**，5 个扩展功能无法加进基础类，故以独立模板类 `dsaac::ExtendedArrayStack<T>` / `ExtendedLinkedStack<T>` 实现（自带存储，接口风格与给定骨架一致）。

| 功能 | 语义 | 复杂度（数组 / 链表） | 对照 `<stack>` |
| --- | --- | --- | --- |
| `clear()` | 删除全部元素 | O(n) / O(n) | `std::stack` 无，需循环 `pop` |
| `swap(other)` | 交换两栈内容 | O(1) / O(1) | 对应 `std::stack::swap` |
| `search(v)` | 元素距栈顶位置（栈顶为 1），未找到 -1 | O(n) / O(n) | `std::stack` 无查找接口 |
| `peek(depth)` | 访问距栈顶第 depth 个元素，越界抛 `out_of_range` | O(1) / O(depth) | `std::stack` 仅 `top` |
| `reverse()` | 原地反转整栈 | O(n) / O(n) | `std::stack` 无 |

`make extend` 自测：两种实现共 **28 项断言全部通过**（含 `peek` 越界的异常检查）。

**与 `std::stack` 的性能对比**（扩展栈 vs `std::stack<int>`，同负载）：

| 实现 | n=100000 | n=1000000 |
| --- | --- | --- |
| `array_stack_ext` | 0.48 ms | 5.06 ms |
| `linked_stack_ext` | 3.03 ms | 31.42 ms |
| `std_stack` | 0.12 ms | 1.20 ms |

**分析**：`std::stack` 快 1 个数量级，原因同上——适配器零开销 + `deque` 分段连续。查阅 [cppreference: std::stack](https://en.cppreference.com/w/cpp/container/stack) 可见其接口刻意精简（`push/pop/top/swap/size/empty`），**不含 `clear/search/peek/reverse`**：这不是疏漏而是适配器哲学——栈只暴露"栈顶操作"的受限视图，任意位置访问本就违背 LIFO 抽象。我们的扩展栈为实验完整性直接实现了这些功能，代价是接口更"胖"、封装边界更模糊（数组的 `peek` 是 O(1)、链表是 O(depth)，两实现复杂度不再一致）。

## 验证记录

- `make`：`test` 与 `stack_extend` 编译通过，`-Wall` 无警告。
- `make check`：case01~case08 全部 PASS；stderr 无交叉校验错误。
- `make extend`：扩展功能自测 28 项全部通过。
- 性能数据取 `-O2 -flto`、每项 3 次取平均的单次运行实测值。