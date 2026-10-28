# 线性表（数组和链表）

## 提交材料导航与运行说明

本节合并原 `INDEX.md` 的材料导航和原 `MANIFEST.md` 的提交要求对照，便于核验三个 Baseline 的代码、样例与实验分析。详细分析、人机协作过程及截图见 [实验报告（PDF）](实验报告.pdf)。

### 材料位置

| 材料 | 位置 | 说明 |
| --- | --- | --- |
| 实验报告 | [实验报告.pdf](实验报告.pdf) | 项目分析、协作过程、运行截图与总结 |
| 构建入口 | [Makefile](Makefile) | 统一编译、样例测试、单元测试与内存检查 |
| 链表基本实现 | [baseline1/](baseline1/) | 裸节点实现、模板链表与命令式测试 |
| 可变数组实现 | [baseline2/](baseline2/) | 动态数组、扩展功能与性能对比 |
| 链表进阶算法 | [baseline3/](baseline3/) | 解环、相交检测与公共路径反转 |

### 编译和运行

在本 README 所在目录执行（需要支持 C++17 的编译器与 `make`）：

```sh
make        # 编译三个 Baseline
make check  # 自动读取各 inputs/，与同名 outputs/ 比对
make unit   # 单元测试和手动构造测试
make asan   # 内存检查，需要编译器支持 AddressSanitizer/UBSan
```

各 Baseline 也可进入对应目录单独执行上述命令。可变数组的完整性能比较运行 `make -C baseline2 bench`。实际实现代码位于各 `baselineN/src/`，无需额外的顶层 `List/src/` 目录。

### Baseline 概览

| Baseline | 主题 | 源码 | 输入与标准输出 | 样例结果 | 单元测试记录 |
| --- | --- | --- | --- | --- | --- |
| baseline1 | 链表基本实现，三个小任务 | [src/](baseline1/src/) | [inputs/](baseline1/inputs/) / [outputs/](baseline1/outputs/) | 3/3 PASS | 46 项断言 |
| baseline2 | 可变数组，四个小任务 | [src/](baseline2/src/) | [inputs/](baseline2/inputs/) / [outputs/](baseline2/outputs/) | 7/7 PASS | 28 项断言 |
| baseline3 | 幽灵交叉路口 | [src/](baseline3/src/) | [inputs/](baseline3/inputs/) / [outputs/](baseline3/outputs/) | 8/8 PASS | 6 项断言 |

样例结果已在提交版副本中重新编译核验；单元测试、性能统计与内存检查结果沿用项目文档和实验报告中的记录，运行上述命令可复核。

## 提交要求逐项对照

以下整理自原提交清单；性能统计与内存检查结论为项目已有验证记录。

### 通则（三个 Baseline 共同适用）

| 提交要求 | 证据位置 | 实际结果 |
| :--- | :--- | :--- |
| 测试样例：`inputs/*.txt` 与 `outputs/` 同名标准输出 | `baselineN/inputs/`、`baselineN/outputs/` | 同名成对齐全，编号连续 |
| 名为 `test` 的测试程序，自动读取样例并比对 | `baselineN/`（`make` 生成） | 逐用例输出 `PASS`/`FAIL`，退出码反映结果 |
| 实现代码：`.cpp` 源文件，结构清晰、保留注释 | `baselineN/src/` | 中文注释、`START/END` 实现区间 |
| 构建文件：`Makefile` 一条命令编译 + 样例测试目标 | `baselineN/Makefile` | `make` 编译、`make check` 比对 |
| 编译无警告 | `baselineN/Makefile` 的 `CXXFLAGS` | `-std=c++17 -Wall -Wextra`（baseline2 另加 `-O2`）**无警告** |
| 内存安全 | 各 `baselineN/` 的 `make asan` | ASan/UBSan 检查；顶层 `make asan` 一键运行三个 Baseline |

### baseline1：链表的基本实现

| 任务内容 / 提交要求 | 证据 | 结果 |
| :--- | :--- | :--- |
| ① 编写链表基本实现（节点/初始化/插入/删除/访问/查找）+ 测试程序 | [src/task1_basic_list/](baseline1/src/task1_basic_list/) | `make unit`：小任务 1 断言全过 |
| ② 实现为模板类，支持任意数据类型 + 测试 | [src/task2_linked_list/](baseline1/src/task2_linked_list/) | int / double / char 三类型实例化通过 |
| ③ 按统一输入输出格式的命令式测试程序 | [src/task3_command_test/](baseline1/src/task3_command_test/) | `make check`：3/3 PASS |
| 提交表（样例 / test / 实现代码 / Makefile） | [baseline1/](baseline1/) | 齐全 |
| 程序与结果分析 | [baseline1/README.md](baseline1/README.md) | 含扩展功能对照表、越界语义统一、验证记录 |

### baseline2：可变数组的实现

| 任务内容 / 提交要求 | 证据 | 结果 |
| :--- | :--- | :--- |
| ① 可变数组类（动态扩容 + 随机访问）+ 测试程序 | [src/task1_vector/](baseline2/src/task1_vector/) | `make check`：7/7 PASS；与 `std::vector` 交叉校验一致 |
| ② 评估与 `std::vector` 的差异 | [task2_performance](baseline2/src/task2_performance/) | 5 场景 × 2 规模，24 次均值 ± sd + Welch t 检验 + 95% CI（见 README） |
| ③ 参照 `<vector>` 新增 ≥5 功能 | [task3_extended_vector](baseline2/src/task3_extended_vector/) | 6 个功能，`make unit` 28 项断言通过（含非平凡类型 `std::string`） |
| ④ 分析可变数组与链表的性能差异 | [task4_array_vs_list](baseline2/src/task4_array_vs_list/) | 4 场景 × 2 规模，24 次均值 ± sd + Welch t 检验，`make bench` 运行 |
| 提交表（样例 / test / 实现代码 / Makefile） | [baseline2/](baseline2/) | 齐全 |
| 设计思路 / 差异评估 / 性能分析 | [baseline2/README.md](baseline2/README.md) | 含扩展功能对照表、`pop_back` 边界说明、口径与代表性边界 |

### baseline3：链表的进阶算法解题（幽灵交叉路口）

| 任务内容 / 提交要求 | 证据 | 结果 |
| :--- | :--- | :--- |
| 算法实现：`detectAndRemoveCycle` / `getIntersectionNode` / `reverseSharedPath` 函数模块 | [src/task1_algorithms/](baseline3/src/task1_algorithms/) | 全部 $O(n)$ 时间 / $O(1)$ 空间，禁用哈希表 |
| 手动构造三类边界链表 + 命令行可视化验证 | [src/task2_manual_test/](baseline3/src/task2_manual_test/) | `make unit`：6 项断言全过 |
| 命令式测试程序（统一输入输出格式） | [src/task3_command_test/](baseline3/src/task3_command_test/) | `make check`：8/8 PASS |
| 复杂度规定：$O(n)$ 时间 / $O(1)$ 空间 | [baseline3/README.md](baseline3/README.md) | 复杂度汇总表 + 设计思路 |
| 进阶算法设计思路报告（含时空复杂度） | [baseline3/README.md](baseline3/README.md) | 三挑战逐一说明，含两处易错点推演 |
| 提交表（样例 / test / 实现代码 / Makefile） | [baseline3/](baseline3/) | 齐全 |
| 防泄漏内存管理 | `make asan` | 先解环再释放、共享尾部只释放一次，零泄漏 |

## 报告截图索引

以下截图已嵌入 [实验报告.pdf](实验报告.pdf)，按报告中的图号定位；提交版无需依赖外部 `docs/` 图片文件。

| 报告图号 | 内容 |
| --- | --- |
| 图 | 内容 |
| 图 1 | 提示词：明确 baseline1 的风格与结构约束 |
| 图 2 | 提示词：在现有框架上继续完成 baseline1 |
| 图 3 | 提示词：合并两份说明文档并重整目录 |
| 图 4 | 提示词：统一三个 baseline 的 src 命名 |
| 图 5 | 提示词：启动 baseline2 |
| 图 6 | 提示词：核对 baseline2 是否符合提交要求 |
| 图 7 | 提示词：启动 baseline3 |
| 图 8 | 提示词：核对 baseline3 是否符合提交要求 |
| 图 9 | 提示词：对 `reverseSharedPath` 拟定方案做对抗性审查（只推演指针、不写代码） |
| 图 10 | AI 审查回答：推演出 `9->next = 9` 自环并给出正确顺序 |
| 图 11 | 三个 baseline 的 `make check` 全部通过 |
| 图 12 | baseline1 小任务 1 单元验证输出 |
| 图 13 | baseline1 小任务 2 单元验证输出 |
| 图 14 | baseline2 扩展功能自测与性能统计 |
| 图 15 | baseline3 手动构造测试可视化输出 |
| 图 16 | 复杂度增长实测（`growth_*`）与 Welch t 检验（`STAT`） |
| 图 17a–d | `make asan` 四屏：ASan/LSan 干净退出 |

## 已知边界与不足

- `Vector<T>` 未提供移动语义和迭代器，异常安全保证尚不完整。
