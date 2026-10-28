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
    │   │   ├── linked_list/  ← 任务 4 复用的链表实现（baseline1 小任务 2 副本）
    │   │   │   ├── README.md
    │   │   │   ├── linked_list.hpp / linked_list.cpp
    │   │   └── stats/        ← 性能基准用的统计工具（Welch t 检验，header-only）
    │   │       ├── README.md
    │   │       └── stats.hpp
    │   ├── task1_vector/     ← 小任务 1：可变数组实现 + 命令式测试
    │   │   ├── README.md
    │   │   ├── vector.hpp（header-only：声明 + 模板实现）
    │   │   └── main_basic.cpp        （编译为 test）
    │   ├── task2_performance/ ← 小任务 2：vs std::vector 性能评估
    │   │   ├── README.md
    │   │   └── main_performance.cpp  （编译为 vector_perf）
    │   ├── task3_extended_vector/ ← 小任务 3：扩展功能 + 对比
    │   │   ├── README.md
    │   │   ├── vector.hpp（header-only）
    │   │   └── main_extend.cpp       （编译为 vector_extend）
    │   └── task4_array_vs_list/ ← 小任务 4：vs 链表性能对比
    │       ├── README.md
    │       └── main_compare.cpp      （编译为 array_list_cmp）
    ├── inputs/               ← 自测样例（case01 ~ case07）
    ├── outputs/              ← 与 inputs/ 同名的标准输出
    └── test 等              ← make 生成的可执行程序
```

## 小任务划分

| 小任务 | 目录 | 可执行程序 | 内容 |
| --- | --- | --- | --- |
| 1 | `src/task1_vector/` | `test` | `Vector<T>` 可变数组：连续原始内存 + 2 倍扩容 + placement new（构造/析构配对）+ header-only；命令式测试程序读 `inputs/`、与 `outputs/` 比对，并与 `std::vector` 交叉校验 |
| 2 | `src/task2_performance/` | `vector_perf` | 5 场景 × 2 规模评估可变数组与 `std::vector` 的性能差异 |
| 3 | `src/task3_extended_vector/` | `vector_extend` | 参照 `<vector>` 新增 6 个功能（`pop_back`/`resize`/`swap`/`assign`/`find`/`shrink_to_fit`）+ 自测 + 与 `std::vector` 对比 |
| 4 | `src/task4_array_vs_list/` | `array_list_cmp` | 4 场景 × 2 规模评估可变数组与链表的性能差异 |

四个小任务的层次关系：小任务 1 实现核心 `Vector<T>` → 小任务 2、4 复用它做对比 → 小任务 3 在其基础上扩展。链表侧（小任务 4）复用 baseline1 的 `LinkedList<T>` 副本。

## 扩展功能对照表

课程要求“参照 `<vector>` 添加更多的功能，五个即可”。本提交在核心可变数组之外，于小任务 3 新增 6 个功能；下表集中列出，便于一眼审阅设计用途、对照接口与测试覆盖。

### 核心 `Vector<T>` 相对最小实现新增的接口（小任务 1）

| 接口 | 对照接口 | 语义 | 复杂度 |
| :--- | :--- | :--- | :--- |
| `operator[]` | `std::vector::operator[]` | 无边界检查访问 | $O(1)$ |
| `front` / `back` | `std::vector::front/back` | 首/尾元素，空表抛 `out_of_range` | $O(1)$ |
| `reserve` | `std::vector::reserve` | 预留容量（只增不减） | $O(n)$ |
| `to_vector` | — | 导出元素序列 | $O(n)$ |

### 小任务 3 新增的 6 个扩展功能（`task3_extended_vector/`）

| 功能 | 对照接口 | 语义 | 复杂度 | 覆盖断言（`main_extend.cpp`） |
| :--- | :--- | :--- | :--- | :--- |
| `pop_back` | `std::vector::pop_back` | 删除尾元素，空表抛异常 | $O(1)$ | pop_back 后内容/size、空表抛异常 |
| `resize` | `std::vector::resize` | `n<size` 截断；`n>size` 用 `val` 补足 | $O(n)$ | 扩大补值、缩小截断 |
| `swap` | `std::vector::swap` | 交换内容，仅换指针与计数 | $O(1)$ | swap 后 a/b 内容 |
| `assign` | `std::vector::assign` | 用 `count` 个 `val` 重新填充 | $O(n)$ | assign 后内容/size |
| `find` | Java `ArrayList.indexOf` | 返回首个 `x` 的下标，无则 `-1` | $O(n)$ | find 命中/未命中 |
| `shrink_to_fit` | `std::vector::shrink_to_fit` | 容量收缩到与长度一致 | $O(n)$ | shrink 后容量==长度、内容不变 |

> **`pop_back` 与非平凡类型（已修正）**：`Vector<T>` 采用 `::operator new` + placement new 管理元素，`pop_back` / `erase` / `clear` / `resize` 收缩都会**显式析构**被移除的元素，因此对 `std::string` 等非平凡类型同样保证“构造 / 析构”严格配对、资源即时回收，行为与 `std::vector::pop_back` 一致。`main_extend.cpp` 专门用 `std::string` 覆盖 `pop_back`/`insert`/`erase`/`shrink_to_fit`/`resize`，并在 `make asan`（ASan/LSan）下验证无语义错误。

## 代码风格约定（与学习材料、baseline1 保持一致）

- 命名空间 `dsaac` 包裹实现，再通过 `template using` 暴露别名。
- 头文件默认只放声明（`.hpp`）、实现集中在 `.cpp`；但**模板类 `Vector<T>` 采用 header-only**（实现全在 `vector.hpp`）——这既是 `std::vector` 的做法，也让 `push_back`/`operator[]` 等热路径能被 `-O2` 跨编译单元内联（详见「差异评估」）。非模板代码（`linked_list`）仍保持 `.hpp` 声明 + `.cpp` 实现的拆分，用 `/* START: xxx implementation */` 与 `/* END */` 标注实现区间。
- 注释统一为中文，解释“为什么”而不只是“做了什么”。
- 测试程序逐项打印 `[PASS]` / `[FAIL]` 或断言统计，便于人工核对；性能程序输出规范的 `PERF name n= ops= time_ms=` 行。

## 构建与自测

在 `baseline2/` 目录下执行：

```sh
make            # 一条命令编译生成 4 个程序（test / vector_perf / vector_extend / array_list_cmp）
make check      # 运行样例比对：case01.txt: PASS / ...
make unit       # 运行小任务 3 扩展功能自测（28 项断言）
make bench      # 运行完整性能分析（vector_perf + array_list_cmp，多规模多场景）
make asan       # 用 ASan/LSan 重编并运行 test 与 vector_extend，验证零泄漏
make clean      # 清理构建产物
```

说明：

- 可执行文件名为 `test`（满足提交规范“提交名为 test 的测试程序”）。因目标名 `test` 已被该二进制占用，**运行样例比对的目标名为 `check`**（避免 make 循环依赖）。
- `test` 自动遍历 `inputs/` 与 `outputs/`：功能行逐字符严格比对；`PERF` 行因含随机器变化的运行时间，只校验名称（`vector_like`/`std_vector`/`linked_list`）与格式。
- **功能测试与性能测量解耦**：课程要求 `test` 的功能输出后必须追加 `PERF` 行，故 PERF 行予以保留；但基准结果按“数据规模”在进程内缓存（同一规模只测量一次），示例用例全程仅 3 次基准，`make check` 不再背负重复测量负载。完整的多规模、多场景分析请用 `make bench`（`vector_perf` / `array_list_cmp`）。
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
| case07 | `CLEAR` 后 `CAPACITY` 保持不变（不缩容）、`SIZE`/`PRINT`(EMPTY)/`EMPTY` 归零、清空后继续 `PUSH_BACK` 复用 |

## 验证记录

- `make`：4 个程序编译通过，`-std=c++17 -Wall -Wextra -O2` 无警告。
- `make check`：case01 ~ case07 共 7 个用例全部 PASS。
- `make unit`：小任务 3 扩展功能与边界共 28 项断言全部通过。
- **交叉校验**：`test` 将同一操作序列同时驱动自实现 `Vector<int>` 与 `std::vector<int>`，功能行逐行比对一致（容量行因扩容策略不同而跳过），证明语义与标准库对齐。
- **ASan / LSan 内存检查**：`make asan` 用 `-fsanitize=address,leak` 重编并运行 `test`、`vector_extend` 两个分配/释放堆内存的功能程序，均干净退出、无内存泄漏（性能基准程序 `array_list_cmp` 复用同一套容器代码，规模大、不适合在 ASan 下运行，故不在该目标内）。

---

## 可变数组实现设计思路

**核心思想**：在一段连续堆内存上维护“已用长度 `size_`”与“已分配容量 `capacity_`”两个计数，对外提供随机访问，容量耗尽时重新申请更大的内存并整体搬移。

1. **连续内存 + 双计数**。`data_` 指向 `::operator new` 申请的一段连续**原始内存**，`size_` 记录有效元素、`capacity_` 记录物理容量。二者分离是关键：`size_ < capacity_` 时 `push_back` 只需在 `data_[size_]` 处就地构造，**无需重新分配**；只有 `size_ == capacity_` 时才扩容。这正是“既能随机访问、又能动态扩容”的实现基础——随机访问来自连续内存（`data_[i]` 是 $O(1)$ 指针算术），动态扩容来自容量与长度解耦。

2. **2 倍扩容与均摊 $O(1)$**。扩容时申请 `2 * capacity_` 的新内存、搬移 `size_` 个元素、释放旧内存（首次从 0 扩到 1）。虽然单次扩容是 $O(n)$，但**均摊**到每次 `push_back` 仍是 $O(1)$：从容量 1 扩到 n 的总搬移量是 $1 + 2 + 4 + \dots + n/2 < n$ 的等比级数，即 $\Theta(n)$，平摊到 n 次追加为常数。这与课程 README“样例中的 CAPACITY 输出假设扩容策略为容量翻倍”一致，case03 专门验证了 2→4→8 的翻倍序列。

3. **原始内存 + placement new（构造 / 析构严格配对）**。`data_` 是用 `::operator new` 申请的原始未初始化内存，仅 `[0, size_)` 内是已构造对象；新增用 `placement new` 就地构造，删除（`erase` / `pop_back` / `clear` / `resize` 收缩）显式调用析构，析构函数销毁全部有效元素后再 `::operator delete` 释放内存。这样对 `std::string` 等**非平凡类型**也保证“构造 / 析构”一一配对、资源即时回收——而非“整块 `new T[]` / `delete[]`”那样只能等到整块释放。拷贝构造 / 赋值逐个复制元素，保证两对象独立（避免浅拷贝共享内存的 double free）；`swap` 只交换指针与计数，$O(1)$。

4. **与链表的根本差异**。可变数组的“扩容搬移”是整块连续内存的重新分配；链表的“扩容”是逐个 `new` 节点、无搬移。这一差异直接决定了后文性能对比中两者的优劣分野。

---

## 与 std::vector 的差异评估

小任务 2 实测数据（g++ 15.2、`-O2`、Linux x86-64，每项重复 **24 次**，报告**均值 ± 总体标准差**；比值 = 自实现耗时 / std::vector 耗时，>1 表示自实现更慢）：

| 场景 | n | 可变数组 time_ms | std::vector time_ms | 比值 |
| --- | --- | --- | --- | --- |
| append（尾部追加） | 1000000 | 2.61 ± 0.14 | 2.80 ± 0.00 | **0.93** |
| append_reserve（预留后追加） | 1000000 | 0.35 ± 0.16 | 0.81 ± 0.00 | **0.43** |
| access（随机访问） | 1000000 | 2.52 ± 0.16 | 2.80 ± 0.00 | **0.90** |
| insert_front（头部插入） | 10000 | 2.04 ± 0.02 | 2.01 ± 0.00 | 1.01 |
| erase_front（头部删除） | 10000 | 2.03 ± 0.00 | 2.06 ± 0.00 | 0.99 |

> **口径说明（头部操作的代表性边界）**：`insert_front` / `erase_front` 每次操作需搬移整段，单次为 $O(n)$、连续 $m$ 次为 $O(m^2)$，故基准把规模从 $10^6$ 缩小到 $10^4$（`n/100`）以保证可运行。该场景的比值（≈1.0）**只代表“搬移完全主导”下的相对表现**，不能外推为“大规模下仍等价”——规模增大时两者都按 $O(m^2)$ 增长，比值仍趋近 1，但绝对耗时迅速上升。

小任务 3 扩展数组的追加+删除对比（`vector_ext` vs `std_vector_ext`，24 次均值 ± sd）：n=100000 为 0.45 ± 0.34 vs 0.26 ± 0.00 ms，n=1000000 为 2.62 ± 0.12 vs 2.80 ± 0.01 ms。

**差异分析**（数据背后体现的实现差异）：

1. **热路径已与 `std::vector` 持平甚至反超（比值 0.43~1.01）**。`Vector<T>` 现为 **header-only**（模板实现全在 `vector.hpp`），因此 `push_back` / `operator[]` 等热路径可被 `-O2` **跨编译单元内联**，与 `std::vector` 的实现方式一致——`append` / `access` 比值降至 0.93 / 0.90，`append_reserve` 甚至快一倍以上（0.43：自实现的追加退化为“写一个 int + 递增”，少了标准库 allocator 抽象与异常安全分支的开销）。

   **如何定位到“内联”这个根因（两轮实验）**：早期版本把模板实现放在 `vector.cpp` 并显式实例化，`main_performance.cpp` 只见声明、无法内联，热路径慢 2~3 倍（`append_reserve` 3.1×、`access` 2.3×）。第一轮用 **LTO**（`g++ -O2 -flto`）强制跨单元内联，比值骤降至 0.43 / 0.90，据此锁定根因是**调用未内联**而非算法；第二轮直接把实现搬进头文件（header-only），在**不加 LTO** 的默认构建下即复现同样结果——即上表。这也修正了一处早先的误判：曾把差距归因于“`new T[]` 需默认初始化”，其实 `new int[n]` 是默认初始化（对 `int` 不保证零初始化），且实现早已改用 `::operator new` + placement new，本就没有多余初始化。

2. **头部操作比值趋近 1.0**：`insert_front`/`erase_front` 的耗时被 $O(n)$ 的元素搬移完全主导，内联带来的常数差异被淹没，故两者几乎持平（0.99~1.01）。

3. **缺少 move 语义与迭代器接口**。`std::vector` 的 `insert`/`erase`/扩容对可移动类型会调用移动构造，且提供迭代器使算法库可直接操作；本实现按值拷贝、只暴露下标接口，功能上是教学用的最小可用集。

4. **结论**。自实现与 `std::vector` 在**算法复杂度上完全一致**（同为均摊 $O(1)$ 追加、$O(1)$ 随机访问、$O(n)$ 中间插删）；且在默认 `-O2` 构建下**热路径的常数开销也已消除**（header-only 内联），多数场景持平、预留追加反超。与标准库的剩余差距只在 move 语义与迭代器接口这类“功能覆盖”上，不影响本任务的性能结论。

**复杂度实测**（不只看绝对耗时，还看耗时随规模的增长；`make bench` 会输出 `PERF growth_*` 行）：

| 场景 | 规模序列 | 各规模耗时（ms） | 规模翻倍时的倍数 |
| --- | --- | --- | --- |
| `append_reserve`（预留后追加） | 250k / 500k / 1M | 0.09 / 0.17 / 0.33 | 1.92× / 2.00× |
| `insert_front`（头部插入） | 2.5k / 5k / 10k | 0.13 / 0.51 / 2.04 | 3.98× / 4.03× |

规模翻倍时 `append_reserve` 耗时近翻倍（≈2×，印证均摊 $O(1)$），`insert_front` 耗时近 4 倍（≈$m^2$，印证头部插入 $O(n)$ 搬移），实测增长率与复杂度分析一致。

**统计显著性**（`make bench` 还会输出 `STAT` 行）。对各场景两组独立样本做 **Welch 两样本 t 检验**（$H_0$：均值相等）并给出均值差的 95% 置信区间（工具见 [src/common/stats/](src/common/stats/)）：

- 热路径差异统计显著且方向为**自实现更快**：`append` 比值 0.93（$p=2.8\times10^{-10}$，均值差 95% CI `[-0.25, -0.17] ms`）、`append_reserve` 0.43（$p\approx10^{-12}$，CI `[-0.53, -0.39] ms`）、`access` 0.90（$p\approx10^{-8}$，CI `[-0.36, -0.22] ms`）。
- `insert_front` / `erase_front` 比值 ≈1.0、CI 极小（约 `±0.06 ms`）：差异虽统计显著，绝对量却可忽略——提示**统计显著 ≠ 实际重要**。
- 可变数组 vs 链表的 `STAT` 行方向与比值一致：`random` 均值差 CI 达 `[2340, 2348] ms`（数组碾压式领先），`prepend`（m=10000）为 `[-1.79, -1.79] ms`（链表反超，差为负）。

---

## 可变数组 vs 链表的性能分析

小任务 4 实测数据（g++ 15.2、`-O2`、Linux x86-64，每项重复 **24 次**取均值 ± 总体标准差；比值 = 链表耗时 / 可变数组耗时，>1 表示链表更慢）：

| 场景 | n | 可变数组 time_ms | 链表 time_ms | 比值 |
| --- | --- | --- | --- | --- |
| append（尾部追加） | 1000000 | 2.59 ± 0.09 | 37.12 ± 0.25 | 14.35 |
| prepend（头部插入） | 10000 | 2.02 ± 0.01 | 0.28 ± 0.01 | 0.14 |
| traverse（顺序遍历） | 1000000 | 0.80 ± 0.37 | 36.88 ± 0.34 | 45.9 |
| random（随机访问 2000 次） | 1000000 | 0.80 ± 0.35 | 2343.27 ± 4.90 | **2943.7** |

**分析**（体现两种结构的本质差异）：

1. **随机访问：数组碾压式领先（~2944×）**。数组 `at(i)` 是一次指针算术 $O(1)$；链表 `at(i)` 必须从头遍历 $O(n)$。2000 次随机访问在百万级链表上退化为 $O(k \cdot n) \approx 2 \times 10^9$ 次指针跳转，且每次跳转都是一次潜在的缓存缺失。这是二者最根本的差异——**“随机访问”正是可变数组存在的意义**。

2. **尾部追加与顺序遍历：数组快 ~14~46×**。两者算法复杂度相同（追加均摊 $O(1)$、遍历 $O(n)$），差距纯来自内存布局：数组元素连续，一次缓存行载入 16 个 int，CPU 预取持续命中（header-only 后 `operator[]` 内联，遍历还可向量化）；链表每个节点独立 `new`、散布堆中，每次 `next` 跳转都可能缓存缺失，还额外背负每节点 8 字节指针 + 堆分配元数据的空间开销。

3. **头部插入：链表反超（比值 0.13，即链表快约 7~8×）**。这是链表唯一的结构性优势场景：链表 `push_front` 只改两条指针 $O(1)$；数组头部插入要把全部元素后移一格 $O(n)$。小规模（m=1000）时两者相当（比值 ≈1.0），规模增大到 m=10000 后数组的 $O(n)$ 搬移劣势显现（比值 0.13）。

4. **场景选型结论**：

| 场景 | 推荐 | 理由 |
| --- | --- | --- |
| 随机访问 / 按下标读写 | 可变数组 | $O(1)$ vs 链表 $O(n)$，差距可达数百倍 |
| 尾部追加、顺序遍历、内存受限 | 可变数组 | 缓存友好、无逐节点分配与指针开销 |
| 频繁头部插入/删除 | 链表 | $O(1)$ vs 数组 $O(n)$ 搬移 |
| 元素体积大、需地址稳定 | 链表 | 数组扩容会搬移并改变元素地址，链表节点地址恒定 |

一句话：**默认用可变数组，只有当“频繁在头部增删”或“需要节点地址稳定”时才选链表**——这与 C++ 实践中 `std::vector` 远常用于 `std::list` 的经验一致。