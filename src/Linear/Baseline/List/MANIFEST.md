# 提交清单（MANIFEST）：线性表（List）Baseline 1–3

本清单把课程 [README.md](README.md) 中的**每一类提交要求**对应到**证据位置**与**实际结果**，供助教逐条核验。导航索引见 [INDEX.md](INDEX.md)，完整分析见 [实验报告.md](实验报告.md)。

## 通则（三个 Baseline 共同适用）

| 提交要求 | 证据位置 | 实际结果 |
| :--- | :--- | :--- |
| 测试样例：`inputs/*.txt` 与 `outputs/` 同名标准输出 | `baselineN/inputs/`、`baselineN/outputs/` | 同名成对齐全，编号连续 |
| 名为 `test` 的测试程序，自动读取样例并比对 | `baselineN/`（`make` 生成） | 逐用例输出 `PASS`/`FAIL`，退出码反映结果 |
| 实现代码：`.cpp` 源文件，结构清晰、保留注释 | `baselineN/src/` | 中文注释、`START/END` 实现区间 |
| 构建文件：`Makefile` 一条命令编译 + 样例测试目标 | `baselineN/Makefile` | `make` 编译、`make check` 比对 |
| 编译无警告 | `baselineN/Makefile` 的 `CXXFLAGS` | `-std=c++17 -Wall -Wextra`（baseline2 另加 `-O2`）**无警告** |
| 内存安全 | 各 `baselineN/` 的 `make asan` | ASan/LSan 干净退出，无泄漏；顶层 `make asan` 一键跑完三者 |

## baseline1：链表的基本实现

| 任务内容 / 提交要求 | 证据 | 结果 |
| :--- | :--- | :--- |
| ① 编写链表基本实现（节点/初始化/插入/删除/访问/查找）+ 测试程序 | [src/task1_basic_list/](baseline1/src/task1_basic_list/) | `make unit`：小任务 1 断言全过 |
| ② 实现为模板类，支持任意数据类型 + 测试 | [src/task2_linked_list/](baseline1/src/task2_linked_list/) | int / double / char 三类型实例化通过 |
| ③ 按统一输入输出格式的命令式测试程序 | [src/task3_command_test/](baseline1/src/task3_command_test/) | `make check`：3/3 PASS |
| 提交表（样例 / test / 实现代码 / Makefile） | [baseline1/](baseline1/) | 齐全 |
| 程序与结果分析 | [baseline1/README.md](baseline1/README.md) | 含扩展功能对照表、越界语义统一、验证记录 |

## baseline2：可变数组的实现

| 任务内容 / 提交要求 | 证据 | 结果 |
| :--- | :--- | :--- |
| ① 可变数组类（动态扩容 + 随机访问）+ 测试程序 | [src/task1_vector/](baseline2/src/task1_vector/) | `make check`：7/7 PASS；与 `std::vector` 交叉校验一致 |
| ② 评估与 `std::vector` 的差异 | [task2_performance](baseline2/src/task2_performance/) | 5 场景 × 2 规模，24 次均值 ± sd + Welch t 检验 + 95% CI（见 README） |
| ③ 参照 `<vector>` 新增 ≥5 功能 | [task3_extended_vector](baseline2/src/task3_extended_vector/) | 6 个功能，`make unit` 28 项断言通过（含非平凡类型 `std::string`） |
| ④ 分析可变数组与链表的性能差异 | [task4_array_vs_list](baseline2/src/task4_array_vs_list/) | 4 场景 × 2 规模，24 次均值 ± sd + Welch t 检验，`make bench` 运行 |
| 提交表（样例 / test / 实现代码 / Makefile） | [baseline2/](baseline2/) | 齐全 |
| 设计思路 / 差异评估 / 性能分析 | [baseline2/README.md](baseline2/README.md) | 含扩展功能对照表、`pop_back` 边界说明、口径与代表性边界 |

## baseline3：链表的进阶算法解题（幽灵交叉路口）

| 任务内容 / 提交要求 | 证据 | 结果 |
| :--- | :--- | :--- |
| 算法实现：`detectAndRemoveCycle` / `getIntersectionNode` / `reverseSharedPath` 函数模块 | [src/task1_algorithms/](baseline3/src/task1_algorithms/) | 全部 $O(n)$ 时间 / $O(1)$ 空间，禁用哈希表 |
| 手动构造三类边界链表 + 命令行可视化验证 | [src/task2_manual_test/](baseline3/src/task2_manual_test/) | `make unit`：6 项断言全过 |
| 命令式测试程序（统一输入输出格式） | [src/task3_command_test/](baseline3/src/task3_command_test/) | `make check`：8/8 PASS |
| 复杂度规定：$O(n)$ 时间 / $O(1)$ 空间 | [baseline3/README.md](baseline3/README.md) | 复杂度汇总表 + 设计思路 |
| 进阶算法设计思路报告（含时空复杂度） | [baseline3/README.md](baseline3/README.md) | 三挑战逐一说明，含两处易错点推演 |
| 提交表（样例 / test / 实现代码 / Makefile） | [baseline3/](baseline3/) | 齐全 |
| 防泄漏内存管理 | `make asan` | 先解环再释放、共享尾部只释放一次，零泄漏 |

## 过程与 Prompt 证据

| 证据 | 位置 | 说明 |
| :--- | :--- | :--- |
| 10 张 Prompt 截图 | `docs/stage*-prompt-*.png` | 覆盖四个协作阶段（含阶段四对 `reverseSharedPath` 的对抗性审查两屏），见 [INDEX.md](INDEX.md) 证据索引 |
| 10 张运行结果截图 | `docs/result-*.png` | `make check` 全过 + 各 Baseline 单元/手动测试、复杂度增长与 Welch 检验、ASan 四屏 |

## 已知边界与不足（主动披露）

- `Vector<T>` 采用 `::operator new` + placement new，`pop_back` / `erase` / `clear` / `resize` 收缩均**显式析构**元素；对非平凡类型（`std::string` 已验证）构造 / 析构严格配对，详见 [baseline2/README.md](baseline2/README.md)。
- 裸节点 `access` 对负下标统一返回 `nullptr`（与教程原实现的“返回头节点”不同，为消除与外壳层 `ERROR` 语义的不一致），详见 [baseline1/README.md](baseline1/README.md) 的「越界语义统一」。
- 性能基准每项重复 24 次并报告均值 ± 总体标准差，另附 **Welch 两样本 t 检验与均值差 95% CI**（`make bench` 的 `STAT` 行）；但未固定 CPU 链频率、未做多进程独立复现。`Vector<T>` 尚无 move 语义与迭代器接口。