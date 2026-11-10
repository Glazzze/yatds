# 提交清单（MANIFEST）：字符串（String）Baseline 1–3

本清单把课程 [README.md](README.md) 中的**每一类提交要求**对应到**证据位置**与**实际结果**，供助教逐条核验。导航索引见 [INDEX.md](INDEX.md)，完整分析见 [实验报告.md](实验报告.md)。

## 通则（三个 Baseline 共同适用）

| 提交要求 | 证据位置 | 实际结果 |
| :--- | :--- | :--- |
| 测试样例：`inputs/*.txt` 与 `outputs/` 同名标准输出 | `baselineN/inputs/`、`baselineN/outputs/` | 编号连续，逐同名成对：5 / 6 / 6 组 |
| 名为 `test` 的测试程序，自动读取样例并比对 | `baselineN/test`（`make` 生成） | 自动遍历、逐用例 `PASS` / `FAIL`，退出码反映结果；支持单用例调试 |
| 实现代码：`.cpp` 源文件，结构清晰、保留注释 | `baselineN/src/` | `dsaac` 命名空间、中文注释、`START/END` 实现区间 |
| 构建文件：`Makefile`，一条命令编译 + 样例测试目标 | `baselineN/Makefile` | `make` 编译，`make check` 比对；另有 `unit` / `bench` / `asan` / `clean` |
| 编译无警告 | `baselineN/Makefile` 的 `CXXFLAGS` | `-std=c++17 -Wall -Wextra`（`test` 与性能程序 `-O2`）**无警告** |
| 内存安全 | `baselineN/Makefile` 的 `make asan` | ASan/LSan 干净退出；顶层 `make asan` 一键跑完三者 |
| 程序与结果分析（设计思路、复杂度、性能） | 各 `baselineN/README.md`、[实验报告.md](实验报告.md) | 含公式推导、数据表与增长趋势解读 |

## baseline1：字符串的基本实现

| 任务内容 / 提交要求 | 证据 | 结果 |
| :--- | :--- | :--- |
| ① 基于动态数组的字符串类：`length` / `append` / `substr` | [src/my_string/](baseline1/src/my_string/) | RAII、深拷贝 / 移动、2 倍扩容，`make unit` 覆盖 |
| ② 参照 `std::string` 新增 ≥5 个功能 | [my_string.hpp](baseline1/src/my_string/mystring.hpp) 扩展接口表 | 实际 15 组扩展接口（`find` / `rfind` / `replace` / `compare` / `insert` / `erase` / 运算符等） |
| ③ 与 `std::string` 的性能对比 + 扩容策略分析 | [task3_performance/](baseline1/src/task3_performance/)（`make bench`） | 100 万次拼接、+1 / 1.5× / 2× 扩容、`reserve` 对比；含 LTO 对照实验 |
| 命令式测试程序（统一 I/O） | [task4_command_test/](baseline1/src/task4_command_test/) | `make check`：5/5 PASS；功能行精确比对，PERF 行校验格式 |
| 提交表（样例 / test / 实现代码 / Makefile） | [baseline1/](baseline1/) | 齐全 |

## baseline2：模式匹配三步递进

| 任务内容 / 提交要求 | 证据 | 结果 |
| :--- | :--- | :--- |
| Step 1 朴素算法与极端用例分析 | [task1_naive_analysis/](baseline2/src/task1_naive_analysis/) | `A^(n-1)B` 对 `A^(m-1)B`；n、m 翻倍时比较次数约 4 倍，实测 $O(nm)$ |
| Step 2 KMP 实现与性能对照 | [pattern_match/](baseline2/src/pattern_match/)、同一 bench | KMP 比较次数约 $2n$，n=40 000 / m=400 时比较次数为朴素的 1/199 |
| Step 3 `nextval` 实现与常数级优化 | [task3_nextval/](baseline2/src/task3_nextval/) | 块状主串上比较次数 3 997 000 → 2 001 998（0.501 倍），耗时约减半 |
| `next` / `nextval` 与课程样例一致 | [task2_kmp/](baseline2/src/task2_kmp/)（`make unit`） | `AAAAB → 0 1 2 3 0 / 0 0 0 0 0`；另提供扫描安全回退表并说明差异 |
| 命令式测试（含 PERF 统计） | [task4_command_test/](baseline2/src/task4_command_test/) | `make check`：6/6 PASS；每例追加三行 PERF |
| 提交表（样例 / test / 实现代码 / Makefile） | [baseline2/](baseline2/) | 齐全 |

## baseline3：双链 DNA 目标蛋白编码片段检索

| 任务内容 / 提交要求 | 证据 | 结果 |
| :--- | :--- | :--- |
| 高效字符串匹配（不调用自带查找） | [dna_search.cpp](baseline3/src/dna_search/dna_search.cpp) | 自实现前缀函数 + KMP，支持重叠匹配；第一行输出 `pi` |
| 正向 / 反向互补 × 3 阅读框（共 6 种） | 同上 | 标准遗传密码表 + `reverseComplement` + `translateFrame` |
| 匹配坐标映射（1-based 闭区间、链方向、frame） | 同上 | 正向 / 反向两套公式；官方样例 `2 7 + 2`、`11 16 - 2`、`18 23 + 3` 全部吻合 |
| 区间查询优化（前缀和） | `answerQueries` | 预处理 $O(n+K)$，单次查询 $O(1)$，总体 $O(n+m+q)$ |
| 命令式测试（统一 I/O） | [task2_command_test/](baseline3/src/task2_command_test/) | `make check`：6/6 PASS，输出完全确定、逐字符比对 |
| 独立正确性验证 | [tools/oracle.py](baseline3/tools/oracle.py) | 300 轮随机差分全部一致 |
| 提交表（样例 / test / 实现代码 / Makefile） | [baseline3/](baseline3/) | 齐全 |

## 过程与证据

| 证据 | 位置 | 说明 |
| :--- | :--- | :--- |
| 协作过程说明 | [实验报告.md](实验报告.md) 第二节 | 需求梳理 → 计划确认 → 分 Baseline 实施 → 审查与修正 |
| 截图清单（待补） | [docs/README.md](docs/README.md) | 6 轮 prompt（含 2 轮方向修正）+ 8 张运行结果截图，含文件名、命令与预期画面 |
| 可复现命令 | [INDEX.md](INDEX.md) | `make check` / `unit` / `bench` / `asan` 与 oracle |

## 已知边界与不足（主动披露）

- **两套 `nextval`**：课程样例口径的 `getNextVal`（`AAAAB → 0 0 0 0 0`）不能直接驱动扫描，否则会漏匹配；匹配使用扫描安全的 `getNextValFallback`，差异已在 [baseline2/README.md](baseline2/README.md) 与单元断言中说明。
- **`substr` 边界**：非空串 `pos >= length()` 判越界（即使 `len == 0`），`pos + len` 越界按末尾截断；该语义沿用课程骨架并由 case02 / case03 锁定。
- **性能数据口径**：单机单次运行（g++ 15.2、`-O2`），绝对值随机器 / 负载波动；结论以比较次数、增长趋势与相对比值为主。`test` 内嵌的 PERF 为固定规模单次采样，权威数据来自 `make bench`。
- **截图未附**：prompt 与终端截图无法由代码生成，已给出完整清单与生成命令，待手工补入 [docs/](docs/)。
- **DNA 覆盖**：随机差分与样例覆盖小规模输入；未对 $n=10^6$、$q=2\times10^5$ 的极限规模做压测，复杂度分析为理论上界。