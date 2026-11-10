# 字符串 Baseline 1 提交说明：字符串的基本实现

本目录（`baseline1/`）完成 [课程 README](../README.md) 中 Baseline 任务 1 的三个小任务：基于动态数组实现字符串类 `MyString`（长度 / 拼接 / 求子串 + 参照 `std::string` 的扩展功能），评估其与 `std::string` 的性能差异，并分析扩容策略对连续拼接的影响。课程 README 与课程骨架 `../src/` 均保持原样，本目录自包含。

## 目录结构

```text
baseline1/
├── README.md                   ← 本提交说明
├── Makefile                    ← make / check / unit / bench / asan / clean
├── inputs/case01 ~ case05.txt  ← 自测样例（case01 为课程官方样例）
├── outputs/case01 ~ case05.txt ← 与 inputs 同名的标准输出
├── test                        ← make 生成的测试程序（提交名 test）
└── src/
    ├── my_string/
    │   ├── mystring.hpp        ← MyString 类声明
    │   └── mystring.cpp        ← MyString 实现（START/END 标注）
    ├── task2_extended/
    │   └── main_extended.cpp   ← 扩展功能单元验证（70 项断言）
    ├── task3_performance/
    │   └── main_performance.cpp← 连续拼接与扩容策略实验（PERF 行）
    └── task4_command_test/
        └── main_test.cpp       ← 命令式测试程序（编译为 test）
```

## 小任务划分

| 小任务 | 目录 / 程序 | 内容 |
| :--- | :--- | :--- |
| 1 | `src/my_string/` | 核心 `MyString`：`length` / `append` / `substr`，连续堆内存 + 2 倍扩容 + RAII |
| 2 | `src/task2_extended/`（`make unit`） | 参照 `std::string` 的扩展功能与边界断言，共 70 项 |
| 3 | `src/task3_performance/`（`make bench`） | 100 万次连续拼接、扩容策略（+1 / 1.5× / 2×）随规模变化 |
| 4 | `src/task4_command_test/`（`make check`） | 统一 I/O 的命令式测试程序，自动遍历 `inputs/` 与 `outputs/` |

## 扩展功能对照表

课程要求“参照 `std::string` 添加更多功能，五个即可”。实际提供以下接口：

| 接口 | 对照 `std::string` | 语义 | 复杂度 |
| :--- | :--- | :--- | :--- |
| `size` / `empty` / `capacity` | 同名 | 长度 / 是否为空 / 已分配槽位 | $O(1)$ |
| `c_str` | `c_str` | 返回以 `'\0'` 结尾的字符数组 | $O(1)$ |
| `reserve` | `reserve` | 预留至少 `newCap` 个字符槽位（只增不减） | $O(n)$ |
| `clear` | `clear` | 清空内容但保留容量 | $O(1)$ |
| `find` | `find` | 首次出现下标，不存在返回 `npos`，空模式返回 0 | $O(nm)$ 最坏 |
| `rfind` | `rfind` | 最后一次出现下标 | $O(nm)$ 最坏 |
| `replace` | `replace` | 替换首次出现的 `oldStr`，不存在或 `oldStr` 为空时不变 | $O(n)$ |
| `compare` | `compare` | 字典序比较，返回 `0 / -1 / 1` | $O(n)$ |
| `insert` | `insert` | 在下标 `pos` 处插入，`pos > length()` 抛 `std::out_of_range` | $O(n)$ |
| `erase` | `erase` | 删除从 `pos` 起的至多 `n` 个字符，越界抛异常 | $O(n)$ |
| `operator[]` | `operator[]` | 无边界检查读写 | $O(1)$ |
| `operator==` / `!=` / `<` / `>` | 同名 | 字典序比较 | $O(n)$ |
| `operator+` / `+=` | 同名 | 拼接生成新串 / 原地追加 | $O(n)$ / 均摊 $O(1)$ |
| 拷贝 / 移动构造与赋值 | 同名 | 深拷贝、自赋值安全、移动后源对象为空 | $O(n)$ / $O(1)$ |
| `setGrowthFactor` / `reallocationCount` | — | 供扩容策略实验观测（扩因子、重分配次数） | $O(1)$ |

## 命令式测试的 I/O 语义

输入：第 1 行初始串（空串写 `EMPTY`），第 2 行操作数 `m`，随后 `m` 行操作。

| 操作 | 输出 / 语义 |
| :--- | :--- |
| `PRINT` | 打印当前串；空串打印 `EMPTY` |
| `LENGTH` | 当前长度 |
| `APPEND t` | 直接拼接 `t`（**不自动加空格**），无输出 |
| `SUBSTR pos len` | 非空串 `pos >= length()` 或 `pos`/`len` 为负输出 `ERROR`；`pos + len` 越界按末尾截断；空串任意 `pos` 返回空串 |
| `FIND t` | 首次出现下标（0 开始），不存在输出 `-1` |
| `REPLACE old new` | 只替换首次出现；未找到则不变，无输出 |
| `COMPARE t` | 输出 `0` / `-1` / `1` |
| `CLEAR` | 清空，无输出 |

功能行之后按课程要求追加三行 `PERF`：

```text
PERF my_string n=100000 ops=200000 time_ms=…
PERF std_string n=100000 ops=200000 time_ms=…
PERF my_string_reserve n=100000 ops=200000 time_ms=…
```

**比对口径**：功能行逐字符精确比对；`PERF` 行只校验名称与 `n=` / `ops=` / `time_ms=` 字段，忽略随机器变化的数值——样例输出中的 `time_ms` 只是某次运行的快照。内嵌 PERF 是固定规模（10 万次追加）的单次采样，**权威性能数据以 `make bench` 为准**（见下节）。

## 构建与自测

在 `baseline1/` 目录下执行：

```sh
make          # 编译生成 test（-std=c++17 -Wall -Wextra -O2）
make check    # 运行样例比对：case01 ~ case05
make unit     # 运行扩展功能单元验证（70 项断言）
make bench    # 运行连续拼接与扩容策略实验
make asan     # ASan/LSan 下重编并运行 test 与单元验证
make clean    # 清理构建产物
```

也支持单用例调试：`./test inputs/case01.txt` 打印该样例的完整输出。

## 测试样例说明

| 样例 | 覆盖场景 |
| :--- | :--- |
| case01 | 课程官方样例原样（`PRINT` / `LENGTH` / `APPEND` / `SUBSTR` / `FIND` / `REPLACE` / `COMPARE`） |
| case02 | 空串全流程：`EMPTY` → `APPEND` → `SUBSTR` → `CLEAR`；`SUBSTR 5 1` 在长度 5 时判越界 |
| case03 | `SUBSTR` 边界（尾部截断、`pos == length`、长度为 0）与 `FIND` 命中 / 未命中；`CLEAR` 后查找 |
| case04 | `REPLACE` 重复替换 / 未命中、`COMPARE` 三种大小关系 |
| case05 | 重复字符长串上的 `FIND` / `APPEND` / `SUBSTR` / `CLEAR` 后再操作 |

## 验证记录

- `make`：编译通过，`-std=c++17 -Wall -Wextra -O2` 无警告。
- `make check`：case01 ~ case05 共 5 个用例全部 PASS。
- `make unit`：扩展功能与边界共 **70 项断言**全部通过，覆盖构造 / 深拷贝 / 自赋值、扩容、`substr` 边界、`find` / `rfind`、`replace`（含 `new` 引用本对象）、`compare` 与比较运算符、`insert` / `erase` 越界、`clear` 复用等。
- `make asan`：`test` 与单元程序在 `-fsanitize=address,leak` 下干净退出，无泄漏 / 越界。

## 性能分析

环境：g++ 15.2.0、`-O2`、Linux x86-64（Intel Xeon Platinum 8358P），`std::chrono::steady_clock` 计时，每项重复 **5 次取平均**。`make bench` 输出如下（示例运行）：

### 1. 连续拼接 100 万次单字符

| 实现 | time_ms | 相对 `std::string` |
| :--- | ---: | ---: |
| `MyString`（2 倍扩容） | 7.888 | 2.22× |
| `std::string` | 3.547 | 1.00× |
| `MyString` + `reserve(1e6+1)` | 5.451 | 1.54× |

### 2. 扩容策略：重分配次数与耗时

| 规模 n | +1（exact）time_ms / reallocs | 1.5 倍 time_ms / reallocs | 2 倍 time_ms / reallocs |
| ---: | ---: | ---: | ---: |
| 2 000 | 0.071 / 2 000 | 0.012 / 19 | 0.011 / 11 |
| 4 000 | 0.191 / 4 000 | 0.023 / 21 | 0.022 / 12 |
| 8 000 | 0.507 / 8 000 | 0.044 / 22 | 0.043 / 13 |
| 16 000 | 1.441 / 16 000 | 0.087 / 24 | 0.086 / 14 |

**解读**：

1. **`+1` 扩容是 $O(n^2)$**：每次追加都重新分配并搬移全部内容，总搬移量 $1 + 2 + \dots + n = \Theta(n^2)$。重分配次数严格等于 n；规模翻倍时耗时约 2.7~2.8 倍（小规模时固定开销占比高，随 n 增大逐步逼近 4 倍），与二次增长一致。
2. **等比扩容均摊 $O(1)$**：1.5 倍与 2 倍的耗时随规模翻倍而**恰好翻倍**（0.012 → 0.023 → 0.044 → 0.087，0.011 → 0.022 → 0.043 → 0.086），即线性增长；重分配次数分别为 $\lceil\log_{1.5} n\rceil$ 与 $\lceil\log_2 n\rceil$ 量级（16 000 时仅 24 / 14 次）。
3. **1.5 倍 vs 2 倍**：重分配次数略多（24 vs 14），但耗时几乎相同——在小规模下重分配次数差异可忽略；2 倍策略胜在重分配更少，1.5 倍策略胜在内存峰值更小，二者都是可接受的等比策略。

### 3. 百万规模对比

| 场景 | time_ms |
| :--- | ---: |
| 1.5 倍扩容 | 5.500 |
| 2 倍扩容 | 5.393 |
| 2 倍扩容 + `reserve` | 5.661 |
| `std::string` | 3.538 |

在百万规模下三种 `MyString` 配置都已做足扩容，`reserve` 不再显著领先（差异在测量噪声内），说明**瓶颈已不在重分配，而在每次追加的固定开销**。

### 4. 与 `std::string` 差距的来源（两轮对照实验）

`MyString` 的实现位于 `mystring.cpp`（课程骨架要求 `.hpp` 声明 / `.cpp` 实现），基准程序在另一个编译单元，`-O2` **无法跨编译单元内联** `append`；而 `std::string::append` 是头文件内联的。为定位根因，额外用 `-O2 -flto` 重新编译同一基准：

| 构建方式 | `MyString` time_ms | `std::string` time_ms |
| :--- | ---: | ---: |
| `-O2`（默认） | 7.888 | 3.547 |
| `-O2 -flto` | **2.954** | 3.554 |

开启 LTO 后 `MyString` 反超 `std::string`（2.95 vs 3.55 ms），说明默认构建下 2.2 倍的差距主要来自**跨编译单元调用未内联**，而非算法或数据结构差异。这与 List baseline2 对 `Vector<T>` 的结论一致：热路径内联后常数差距基本消失。算法层面，二者容量策略同为等比扩容，追加均为**均摊 $O(1)$**。

## 已知边界与取舍

- `substr` 沿用课程骨架语义：非空串 `pos >= length()` 判越界（即使 `len == 0`），`pos + n` 越过末尾时截断；空串任意 `pos` 返回空串。该语义已写入 I/O 说明并由 case02 / case03 锁定。
- `replace` 只替换**首次**出现，与 `std::string::replace(pos, len, str)` 的“按位置替换”语义不同，按课程样例的“替换字符串”理解实现。
- `setGrowthFactor(0)` 表示“每次只扩到刚好够用”，用于演示 $O(n^2)$ 的最坏扩容策略，并非日常推荐配置。
- 性能数据为单机单次运行结果，绝对值随机器与负载波动；报告结论以**增长趋势与相对比值**为准。