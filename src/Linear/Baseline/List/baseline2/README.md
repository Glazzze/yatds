# 线性表（可变数组）Baseline 2 提交说明

本目录（`baseline2/`）完成 [课程 README](../README.md) 中 Baseline 任务 2（可变数组的实现）的四个小任务。课程 README 为原始任务文档，不做改动；本文件合并了目录结构、构建方式、源码组织、代码风格、测试与结果分析（含课程要求的实现设计思路、与 `std::vector` 差异评估、与链表性能分析），作为提交材料的完整说明。

## 目录结构

```text
List/
├── README.md                 ← 课程原始任务文档（未改动）
└── baseline2/                ← 本提交目录（Baseline 任务 2）
    ├── README.md             ← 本提交说明
    ├── Makefile              ← make 生成 4 个程序，make check 运行样例比对
    ├── src/
    │   ├── common/           ← 复用的依赖代码（非任务）
    │   │   └── linked_list/  ← 任务 4 复用的链表实现（baseline1 小任务 2 副本）
    │   │       ├── README.md
    │   │       ├── linked_list.hpp / linked_list.cpp
    │   ├── task1_vector/     ← 小任务 1：可变数组实现 + 命令式测试
    │   │   ├── README.md
    │   │   ├── vector.hpp / vector.cpp
    │   │   └── main_basic.cpp        （编译为 test）
    │   ├── task2_performance/ ← 小任务 2：vs std::vector 性能评估
    │   │   ├── README.md
    │   │   └── main_performance.cpp  （编译为 vector_perf）
    │   ├── task3_extended_vector/ ← 小任务 3：扩展功能 + 对比
    │   │   ├── README.md
    │   │   ├── vector.hpp / vector.cpp
    │   │   └── main_extend.cpp       （编译为 vector_extend）
    │   └── task4_array_vs_list/ ← 小任务 4：vs 链表性能对比
    │       ├── README.md
    │       └── main_compare.cpp      （编译为 array_list_cmp）
    ├── inputs/               ← 自测样例（case01 ~ case06）
    ├── outputs/              ← 与 inputs/ 同名的标准输出
    └── test 等              ← make 生成的可执行程序
```

## 小任务划分

| 小任务 | 目录 | 可执行程序 | 内容 |
| --- | --- | --- | --- |
| 1 | `src/task1_vector/` | `test` | `Vector<T>` 可变数组：连续内存 + 2 倍扩容 + RAII + 随机访问；命令式测试程序读 `inputs/`、与 `outputs/` 比对，并与 `std::vector` 交叉校验 |
| 2 | `src/task2_performance/` | `vector_perf` | 5 场景 × 2 规模评估可变数组与 `std::vector` 的性能差异 |
| 3 | `src/task3_extended_vector/` | `vector_extend` | 参照 `<vector>` 新增 6 个功能（`pop_back`/`resize`/`swap`/`assign`/`find`/`shrink_to_fit`）+ 自测 + 与 `std::vector` 对比 |
| 4 | `src/task4_array_vs_list/` | `array_list_cmp` | 4 场景 × 2 规模评估可变数组与链表的性能差异 |

四个小任务的层次关系：小任务 1 实现核心 `Vector<T>` → 小任务 2、4 复用它做对比 → 小任务 3 在其基础上扩展。链表侧（小任务 4）复用 baseline1 的 `LinkedList<T>` 副本。

## 代码风格约定（与学习材料、baseline1 保持一致）

- 命名空间 `dsaac` 包裹实现，再通过 `template using` 暴露别名。
- 头文件只放声明（`.hpp`），实现集中在同目录 `.cpp`，用 `/* START: xxx implementation */` 与 `/* END */` 标注实现区间。
- 注释统一为中文，解释“为什么”而不只是“做了什么”。
- 测试程序逐项打印 `[PASS]` / `[FAIL]` 或断言统计，便于人工核对；性能程序输出规范的 `PERF name n= ops= time_ms=` 行。

## 构建与自测

在 `baseline2/` 目录下执行：

```sh
make            # 一条命令编译生成 4 个程序（test / vector_perf / vector_extend / array_list_cmp）
make check      # 运行样例比对：case01.txt: PASS / ...
make unit       # 运行小任务 3 扩展功能自测（16 项断言）
make clean      # 清理构建产物
```

说明：

- 可执行文件名为 `test`（满足提交规范“提交名为 test 的测试程序”）。因目标名 `test` 已被该二进制占用，**运行样例比对的目标名为 `check`**（避免 make 循环依赖）。
- `test` 自动遍历 `inputs/` 与 `outputs/`：功能行逐字符严格比对；`PERF` 行因含随机器变化的运行时间，只校验名称（`vector_like`/`std_vector`/`linked_list`）与格式。
- 也支持单用例调试：`./test inputs/case01.txt` 直接打印该样例的完整输出。

## 测试样例说明

| 样例 | 覆盖场景 |
| --- | --- |
| case01 | 课程 README 官方样例原样（含 `CAPACITY` 翻倍验证：4→8） |
| case02 | 空数组 n=0：`SIZE`/`PRINT`(EMPTY)/`EMPTY`/`PUSH_BACK`/`GET` 越界/`CLEAR` |
| case03 | 扩容：capacity=2 连续 `PUSH_BACK` 至 8 个，`CAPACITY` 输出翻倍序列 2,4,8,8 |
| case04 | 越界组合：`GET`/`SET`/`INSERT`/`ERASE` 各越界分支（含负下标）+ 合法边界 `INSERT size x` |
| case05 | 负数与重复元素 + `SET` 就地修改 |
| case06 | 头部 `INSERT`/`ERASE`（O(n) 搬移路径）+ `CLEAR` 后复用 |

## 验证记录

- `make`：4 个程序编译通过，`-std=c++17 -Wall -O2` 无警告。
- `make check`：case01 ~ case06 共 6 个用例全部 PASS。
- `make unit`：小任务 3 扩展功能 16 项断言全部通过。
- **交叉校验**：`test` 将同一操作序列同时驱动自实现 `Vector<int>` 与 `std::vector<int>`，功能行逐行比对一致（容量行因扩容策略不同而跳过），证明语义与标准库对齐。
- **ASan / LSan 内存检查**：`test`、`vector_extend`、`array_list_cmp` 三个管理堆内存的程序在 `-fsanitize=address,leak` 下均干净退出，无内存泄漏。

---

## 可变数组实现设计思路

**核心思想**：在一段连续堆内存上维护“已用长度 `size_`”与“已分配容量 `capacity_`”两个计数，对外提供随机访问，容量耗尽时重新申请更大的内存并整体搬移。

1. **连续内存 + 双计数**。`data_` 指向 `new T[capacity_]` 的连续块，`size_` 记录有效元素、`capacity_` 记录物理容量。二者分离是关键：`size_ < capacity_` 时 `push_back` 只需在 `data_[size_++]` 写入，**无需重新分配**；只有 `size_ == capacity_` 时才扩容。这正是“既能随机访问、又能动态扩容”的实现基础——随机访问来自连续内存（`data_[i]` 是 $O(1)$ 指针算术），动态扩容来自容量与长度解耦。

2. **2 倍扩容与均摊 $O(1)$**。扩容时申请 `2 * capacity_` 的新内存、搬移 `size_` 个元素、释放旧内存（首次从 0 扩到 1）。虽然单次扩容是 $O(n)$，但**均摊**到每次 `push_back` 仍是 $O(1)$：从容量 1 扩到 n 的总搬移量是 $1 + 2 + 4 + \dots + n/2 < n$ 的等比级数，即 $\Theta(n)$，平摊到 n 次追加为常数。这与课程 README“样例中的 CAPACITY 输出假设扩容策略为容量翻倍”一致，case03 专门验证了 2→4→8 的翻倍序列。

3. **RAII 与深拷贝**。构造申请、析构 `delete[]` 释放；拷贝构造/赋值逐个复制元素到新内存，保证两个对象独立——避免裸指针浅拷贝共享内存导致的 double free。`swap` 只交换指针与计数，$O(1)$。

4. **与链表的根本差异**。可变数组的“扩容搬移”是整块连续内存的重新分配；链表的“扩容”是逐个 `new` 节点、无搬移。这一差异直接决定了后文性能对比中两者的优劣分野。

---

## 与 std::vector 的差异评估

小任务 2 实测数据（g++ 15.2、`-O2`、Linux x86-64，每项重复 5 次取平均；比值 = 自实现耗时 / std::vector 耗时，>1 表示自实现更慢）：

| 场景 | n | 可变数组 time_ms | std::vector time_ms | 比值 |
| --- | --- | --- | --- | --- |
| append（尾部追加） | 1000000 | 4.72 | 2.78 | 1.70 |
| append_reserve（预留后追加） | 1000000 | 2.59 | 0.81 | 3.22 |
| access（随机访问） | 1000000 | 7.24 | 2.79 | 2.60 |
| insert_front（头部插入） | 10000 | 2.10 | 2.02 | 1.04 |
| erase_front（头部删除） | 10000 | 2.12 | 2.06 | 1.03 |

小任务 3 扩展数组的追加+删除对比（`vector_ext` vs `std_vector_ext`）：n=100000 为 2.66 vs 0.85 ms，n=1000000 为 9.01 vs 2.84 ms。

**差异分析**（数据背后体现的实现差异）：

1. **随机访问与追加慢 2~3 倍的主因：内存初始化方式**。本实现用 `new T[capacity_]` 申请内存，C++ 语义要求对 `T` 逐个**默认初始化**（`int` 会零初始化）；扩容搬移时又对每个元素赋值。`std::vector` 通过 `std::allocator` 申请**未初始化的原始内存**，仅对实际构造的元素用 placement new 就地构造，`reserve` 后追加时几乎无额外初始化开销。这解释了 `append_reserve` 场景 std::vector 快 3 倍以上——预留容量消除了搬移，其追加退化为纯粹的“写一个 int”，而本实现仍背着初始化的固定成本。

2. **头部操作比值趋近 1.0**：`insert_front`/`erase_front` 的耗时被 $O(n)$ 的元素搬移完全主导，内存分配/初始化方式的差异被淹没，故两者几乎持平（1.03~1.04）。这反证了上面的结论——差距来自常数因子（初始化/分配），而非算法。

3. **缺少 move 语义与迭代器接口**。`std::vector` 的 `insert`/`erase`/扩容对可移动类型会调用移动构造，且提供迭代器使算法库可直接操作；本实现按值拷贝、只暴露下标接口，功能上是教学用的最小可用集。

4. **结论**。自实现与 `std::vector` 在**算法复杂度上完全一致**（同为均摊 $O(1)$ 追加、$O(1)$ 随机访问、$O(n)$ 中间插删），差距集中在**实现质量的常数因子**：allocator 未初始化内存、move 语义、更精细的扩容策略。这也说明“理解可变数组的原理”与“写出和标准库一样快的可变数组”是两件事——后者的价值在于生产，前者的价值在于理解。

---

## 可变数组 vs 链表的性能分析

小任务 4 实测数据（比值 = 链表耗时 / 可变数组耗时，>1 表示链表更慢）：

| 场景 | n | 可变数组 time_ms | 链表 time_ms | 比值 |
| --- | --- | --- | --- | --- |
| append（尾部追加） | 1000000 | 4.56 | 36.91 | 8.10 |
| prepend（头部插入） | 10000 | 2.08 | 0.29 | 0.14 |
| traverse（顺序遍历） | 1000000 | 4.07 | 35.56 | 8.74 |
| random（随机访问 2000 次） | 1000000 | 3.15 | 2344.68 | **743.8** |

**分析**（体现两种结构的本质差异）：

1. **随机访问：数组碾压式领先（~744×）**。数组 `at(i)` 是一次指针算术 $O(1)$；链表 `at(i)` 必须从头遍历 $O(n)$。2000 次随机访问在百万级链表上退化为 $O(k \cdot n) \approx 2 \times 10^9$ 次指针跳转，且每次跳转都是一次潜在的缓存缺失。这是二者最根本的差异——**“随机访问”正是可变数组存在的意义**。

2. **尾部追加与顺序遍历：数组快 ~8×**。两者算法复杂度相同（追加均摊 $O(1)$、遍历 $O(n)$），差距纯来自内存布局：数组元素连续，一次缓存行载入 16 个 int，CPU 预取持续命中；链表每个节点独立 `new`、散布堆中，每次 `next` 跳转都可能缓存缺失，还额外背负每节点 8 字节指针 + 堆分配元数据的空间开销。

3. **头部插入：链表反超（比值 0.14，即链表快约 7×）**。这是链表唯一的结构性优势场景：链表 `push_front` 只改两条指针 $O(1)$；数组头部插入要把全部元素后移一格 $O(n)$。小规模（m=1000）时两者相当（比值 0.92），规模增大后数组的 $O(n)$ 搬移劣势显现。

4. **场景选型结论**：

| 场景 | 推荐 | 理由 |
| --- | --- | --- |
| 随机访问 / 按下标读写 | 可变数组 | $O(1)$ vs 链表 $O(n)$，差距可达数百倍 |
| 尾部追加、顺序遍历、内存受限 | 可变数组 | 缓存友好、无逐节点分配与指针开销 |
| 频繁头部插入/删除 | 链表 | $O(1)$ vs 数组 $O(n)$ 搬移 |
| 元素体积大、需地址稳定 | 链表 | 数组扩容会搬移并改变元素地址，链表节点地址恒定 |

一句话：**默认用可变数组，只有当“频繁在头部增删”或“需要节点地址稳定”时才选链表**——这与 C++ 实践中 `std::vector` 远常用于 `std::list` 的经验一致。