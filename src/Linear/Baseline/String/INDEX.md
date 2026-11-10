# 提交索引：字符串（String）Baseline 1–3

本文件是「任务 → 证据 → 结论」的导航索引，方便助教快速定位每一份材料。逐条需求对应关系见 [MANIFEST.md](MANIFEST.md)，完整分析与协作过程见 [实验报告.md](实验报告.md)。

## 顶层材料

| 材料 | 位置 | 说明 |
| :--- | :--- | :--- |
| 课程原始任务文档 | [README.md](README.md) | 三个 Baseline 的任务 / 格式要求，**未改动** |
| 课程骨架（参考） | [src/](src/) | 课程提供的 `my_string` / `pattern_match` 骨架，**未改动** |
| 实验报告 | [实验报告.md](实验报告.md) | 项目分析、人机协作、审查测试与优化、最终产物 |
| 提交清单 | [MANIFEST.md](MANIFEST.md) | 逐条列出提交要求 ↔ 证据 ↔ 结果 |
| 顶层构建入口 | [Makefile](Makefile) | `make` / `make check` / `make unit` / `make bench` / `make asan` 一键跑完三个 Baseline |
| 截图清单 | [docs/README.md](docs/README.md) | 6 轮 prompt 原文（含 2 轮方向修正）+ 8 张运行截图的文件名、命令与预期画面 |

## Baseline 概览

| Baseline | 主题 | 代码 | 样例 | 构建入口 | 验证结果 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| baseline1 | 字符串基本实现（3 小任务 + 命令式测试） | [baseline1/src/](baseline1/src/) | [inputs](baseline1/inputs/) / [outputs](baseline1/outputs/)（5 例） | [baseline1/Makefile](baseline1/Makefile) | 5/5 PASS，70 项断言，ASan 干净 |
| baseline2 | 模式匹配三步递进（朴素 → KMP → nextval） | [baseline2/src/](baseline2/src/) | [inputs](baseline2/inputs/) / [outputs](baseline2/outputs/)（6 例） | [baseline2/Makefile](baseline2/Makefile) | 6/6 PASS，35 项断言，ASan 干净 |
| baseline3 | 双链 DNA 蛋白编码片段检索 | [baseline3/src/](baseline3/src/) | [inputs](baseline3/inputs/) / [outputs](baseline3/outputs/)（6 例） | [baseline3/Makefile](baseline3/Makefile) | 6/6 PASS，30 项断言，随机差分 300 轮，ASan 干净 |

统一编译参数：`-std=c++17 -Wall -Wextra`；`test` 与性能程序另加 `-O2`。三个 Baseline 实测**无警告**。

## 任务与证据导航

### baseline1：字符串的基本实现

- 核心类与扩展接口：[my_string/](baseline1/src/my_string/)，接口对照表见 [baseline1/README.md](baseline1/README.md)
- 扩展功能断言：[task2_extended/](baseline1/src/task2_extended/)（`make unit`，70 项）
- 性能与扩容策略：[task3_performance/](baseline1/src/task3_performance/)（`make bench`）
- 命令式测试：[task4_command_test/](baseline1/src/task4_command_test/)（`make check`，5/5）
- 结论：2 倍扩容均摊 $O(1)$、`+1` 扩容 $O(n^2)$；默认构建与 `std::string` 的差距主要来自跨编译单元未内联（LTO 对照实验见 README）

### baseline2：模式匹配

- 朴素 / KMP / nextval：[pattern_match/](baseline2/src/pattern_match/)
- 朴素最坏情况与 KMP 对照：[task1_naive_analysis/](baseline2/src/task1_naive_analysis/)（Step 1/2）
- KMP 单元验证：[task2_kmp/](baseline2/src/task2_kmp/)（`make unit`，35 项）
- next / nextval 对比：[task3_nextval/](baseline2/src/task3_nextval/)（Step 3）
- 命令式测试：[task4_command_test/](baseline2/src/task4_command_test/)（`make check`，6/6）
- 结论：最坏用例下朴素比较次数随 $n,m$ 翻倍增至约 4 倍，KMP 约 $2n$；`nextval` 在重复字符场景把比较次数降到 0.501 倍

### baseline3：DNA 蛋白编码片段检索

- 6 阅读框翻译 / KMP / 坐标映射 / 前缀和：[dna_search/](baseline3/src/dna_search/)
- 单元与边界断言：[task1_unit/](baseline3/src/task1_unit/)（`make unit`，30 项）
- 命令式测试：[task2_command_test/](baseline3/src/task2_command_test/)（`make check`，6/6）
- 独立暴力 oracle：[tools/oracle.py](baseline3/tools/oracle.py)（300 轮随机差分）
- 结论：$O(n+m+q)$；官方样例输出逐行一致（含反向链坐标）

## 复现命令

```sh
cd src/Linear/Baseline/String
make all        # 编译三个 test
make check      # 17 个样例全部 PASS
make unit       # 70 + 35 + 30 项断言
make bench      # baseline1 / baseline2 性能实验
make asan       # ASan/LSan 三个 Baseline
python3 baseline3/tools/oracle.py 300 20261110
```