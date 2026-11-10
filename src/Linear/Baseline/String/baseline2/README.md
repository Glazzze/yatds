# 字符串 Baseline 2 提交说明：模式匹配三步递进

本目录（`baseline2/`）完成 [课程 README](../README.md) 中 Baseline 任务 2：从朴素匹配出发，实现 KMP，再用 `nextval` 做进阶优化；要求构造极端用例验证最坏复杂度，并对比三者的时间与字符比较次数。课程 README 与课程骨架 `../src/` 保持原样，本目录自包含。

## 目录结构

```text
baseline2/
├── README.md                    ← 本提交说明
├── Makefile                     ← make / check / unit / bench / asan / clean
├── inputs/case01 ~ case06.txt   ← 自测样例（case01 为课程官方样例）
├── outputs/case01 ~ case06.txt  ← 与 inputs 同名的标准输出
├── test                         ← make 生成的测试程序（提交名 test）
└── src/
    ├── pattern_match/
    │   ├── match.hpp            ← 朴素 / KMP / nextval 接口
    │   └── match.cpp            ← 实现（START/END 标注）
    ├── task1_naive_analysis/
    │   └── main_naive.cpp       ← Step 1/2：朴素最坏情况 vs KMP（PERF 行）
    ├── task2_kmp/
    │   └── main_unit.cpp        ← KMP 单元验证（35 项断言）
    ├── task3_nextval/
    │   └── main_nextval.cpp     ← Step 3：next vs nextval（数组 / 比较次数 / 耗时）
    └── task4_command_test/
        └── main_test.cpp        ← 命令式测试程序（编译为 test）
```

## 三步递进

| 步骤 | 算法 | 核心思想 | 时间 / 空间 |
| :--- | :--- | :--- | :--- |
| 1 | `naiveMatch` | 主串每个起点逐字符比较，失配则主串指针回退 | $O(nm)$ / $O(1)$ |
| 2 | `kmpMatch` | 前缀函数 `next` 记录模式串自重叠长度，主串指针**永不回退** | $O(n+m)$ / $O(m)$ |
| 3 | `kmpNextValMatch` | `nextval` 跳过“回退后仍相同”的无效比较，进一步减少常数 | $O(n+m)$ / $O(m)$ |

## 两套 `nextval`：课程样例口径 vs 扫描安全形式

这是本 Baseline 最重要的设计说明。课程骨架的 `next` 采用**前缀函数**约定：

```text
next[j] = P[0..j] 的最长真前缀同时也是后缀的长度，next[0] = 0
```

课程官方样例要求 `AAAAB` 的 `NEXTVAL` 输出 `0 0 0 0 0`。按常见教材的 `nextval[j] = next[next[j]]（当 P[j] == P[next[j]]）` 递推确实得到该结果，但它**不能直接驱动扫描**：当 `P[j]` 与主串失配、且 `P[j] == P[next[j]]` 时，`nextval` 指向的位置字符仍相同，必然再次失配，数值上相当于“多跳了一步”。若直接拿它做失败回退，会漏掉匹配。

为同时满足“输出课程样例要求的数组”和“匹配必须正确”，本提交提供两张表：

| 接口 | 用途 | `AAAAB` | `AAAAA` | `ABAB` | `AABAAAB` |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `getNext` | 前缀函数（KMP 扫描使用 `next[matched-1]`） | `0 1 2 3 0` | `0 1 2 3 4` | `0 0 1 2` | `0 1 0 1 2 2 3` |
| `getNextVal` | **课程样例口径**，命令式 `NEXTVAL` 输出 | `0 0 0 0 0` | `0 0 0 0 0` | `0 0 1 2` | `0 0 0 0 2 2 3` |
| `getNextValFallback` | **扫描安全**的失败回退表，`kmpNextValMatch` 使用 | `0 0 0 0 3` | `0 0 0 0 0` | `0 0 0 0` | `0 0 1 0 0 2 1` |

`getNextValFallback` 的递推为：从 `k = next[j-1]` 出发，若 `P[j] == P[k]` 则沿 `next[k-1]` 继续回退，直到字符不同或 `k == 0`，取此时的 `k`。它在 `AAAAB` 上的最后一位是 3（失配于 `B` 后回退到 `AAAA` 的末位），保证扫描不漏匹配。两表差异已在 `main_unit.cpp` 中逐项断言，并用独立的 `referenceMatch` 暴力实现交叉验证 17 组主串 / 模式串的匹配结果一致。

## 命令式测试的 I/O 语义

输入：第 1 行 `text`，第 2 行 `pattern`，第 3 行操作数 `m`，随后 `m` 行操作。

| 操作 | 输出 |
| :--- | :--- |
| `NEXT` | 前缀函数数组，空格分隔 |
| `NEXTVAL` | 课程样例口径的 `nextval` 数组 |
| `NAIVE` | 朴素匹配首次出现下标（0 开始），不存在 `-1` |
| `KMP` | KMP 首次出现下标，不存在 `-1` |
| `KMP_NEXTVAL` | 用扫描安全回退表的 KMP 首次出现下标 |
| `ALL` | 依次输出 `NAIVE`、`KMP`、`KMP_NEXTVAL` 三个首次匹配下标（三行） |

空模式串视为在位置 0 匹配成功（与 `std::string::find` 一致）；模式串长于主串时各匹配操作输出 `-1`。

功能行之后追加三行 `PERF`：

```text
PERF naive n=… m=… time_ms=…
PERF kmp n=… m=… time_ms=…
PERF kmp_nextval n=… m=… time_ms=…
```

**比对口径**：功能行精确比对；`PERF` 行只校验名称与 `n=` / `m=` / `time_ms=` 格式，忽略数值。详细性能分析见 `make bench`。

## 构建与自测

在 `baseline2/` 目录下执行：

```sh
make          # 编译生成 test（-std=c++17 -Wall -Wextra -O2）
make check    # 运行样例比对：case01 ~ case06
make unit     # 运行 KMP / nextval 单元验证（35 项断言）
make bench    # 运行朴素最坏情况、KMP 对照与 nextval 优化实验
make asan     # ASan/LSan 下重编并运行 test 与单元验证
make clean    # 清理构建产物
```

单用例调试：`./test inputs/case01.txt`。

## 测试样例说明

| 样例 | 覆盖场景 |
| :--- | :--- |
| case01 | 课程官方样例原样：`AAAAAAAAAAB` / `AAAAB`，锁定 `NEXT 0 1 2 3 0`、`NEXTVAL 0 0 0 0 0` 与三种匹配结果 6 |
| case02 | 模式串长于主串（`ABCABCABC` / `ABCD`）：数组正常，匹配全部 `-1` |
| case03 | 重叠匹配：`AAAAA` / `AA`，`NEXT 0 1`、`NEXTVAL 0 0`，三算法首次命中均为 0 |
| case04 | 单字符 `A` 上查询 `AA`：模式串更长，全部 `-1` |
| case05 | 长文本（19 个 A + B）上 `AAAAB` 的匹配：三种算法与 `ALL` 一致输出 15 |
| case06 | 最坏情况的缩小版：`A^10 B` / `A^5`，全部 `-1`，用于校验回退路径 |

## 验证记录

- `make`：编译通过，`-std=c++17 -Wall -Wextra -O2` 无警告。
- `make check`：case01 ~ case06 共 6 个用例全部 PASS；case01 与课程样例逐行一致。
- `make unit`：35 项断言全部通过，覆盖三张表的逐项取值、`nextval` 与扫描安全表的差异、17 组主串 / 模式串的匹配结果与独立暴力实现一致、空主串 / 空模式串 / 模式串更长等边界、最坏情况下 KMP 比较次数远少于朴素。
- `make asan`：`test` 与单元程序干净退出，无泄漏 / 越界。

## 性能分析

环境：g++ 15.2.0、`-O2`、Linux x86-64（Intel Xeon Platinum 8358P）。每项重复 3~5 次取平均；字符比较次数由算法内部计数器给出，**不受机器与优化影响**，是更稳定的复杂度证据。

### Step 1 / 2：最坏用例下朴素 vs KMP

用例：`text = 'A'^(n-1)+'B'`，`pattern = 'A'^(m-1)+'B'`；朴素算法在几乎每个起点都要比较到模式串末尾才失配。

| n | m | 朴素 time_ms / 比较次数 | KMP time_ms / 比较次数 |
| ---: | ---: | ---: | ---: |
| 10 000 | 100 | 0.749 / 990 100 | 0.032 / 19 900 |
| 20 000 | 200 | 2.923 / 3 960 200 | 0.060 / 39 800 |
| 40 000 | 400 | 10.493 / 15 840 400 | 0.137 / 79 600 |

**解读**：n、m 同时翻倍时，朴素比较次数约变为 4 倍（990 100 → 3 960 200 → 15 840 400），耗时约 3.9 倍 / 3.6 倍，符合 $O(nm)$；KMP 比较次数约等于 $2n$（19 900 → 39 800 → 79 600），耗时约 2.1 倍，符合 $O(n+m)$。在 n=40 000、m=400 时，朴素已是 KMP 的约 76 倍耗时、199 倍比较次数。

### Step 3：`next` vs `nextval`（块状主串）

用例：`pattern = 'A'^1000`；主串由“999 个 A + 1 个 B”的块重复 2 000 次构成（长度 1 999 999），任何位置都不构成完整匹配。原始 `next` 在每块末尾失配后需沿失败链连续回退 999 次，`nextval` 回退表可一步归零。

| 算法 | time_ms | comparisons | 相对比较次数 |
| :--- | ---: | ---: | ---: |
| KMP（`next`） | 7.554 | 3 997 000 | 1.000 |
| KMP + 扫描安全 `nextval` | 3.025 | 2 001 998 | **0.501** |

**解读**：`nextval` 把回退过程中的重复字符比较跳掉，比较次数减半（0.501），耗时同步减半（7.55 → 3.03 ms）。注意优化的是**常数级**：二者时间复杂度同为 $O(n+m)$，`nextval` 不改变渐进复杂度，只在“模式串含大量相同字符”这类输入上减少无效回退。

### 三算法复杂度汇总

| 算法 | 预处理 | 匹配 | 空间 | 最坏比较次数 |
| :--- | :--- | :--- | :--- | :--- |
| 朴素 | — | $O(nm)$ | $O(1)$ | $(n-m+1)m$ |
| KMP | $O(m)$ | $O(n)$ | $O(m)$ | $\le 2n$ |
| KMP + `nextval` | $O(m)$ | $O(n)$ | $O(m)$ | $\le 2n$，重复字符场景常数更小 |

## 已知边界与取舍

- `nextval` 按课程样例口径输出，与扫描安全回退表分开提供；直接拿 `getNextVal` 驱动扫描会漏匹配，因此测试与匹配统一使用 `getNextValFallback`。这一取舍已在 README 与单元断言中显式说明。
- 空模式串按 `std::string::find` 语义视为在位置 0 匹配，`ALL` 此时输出三行 `0`。
- 字符比较次数的统计包含预处理之外的主循环比较；表构造本身为 $O(m)$，在 n 很大时对总时间影响可忽略。
- 性能数据为单机单次运行结果，绝对值随机器波动；结论以比较次数与增长趋势为准。