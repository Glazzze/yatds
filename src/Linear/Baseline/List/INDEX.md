# 提交索引：线性表（List）Baseline 1–3

本文件是「任务 → 证据 → 结论」的导航索引，方便助教在不逐页翻阅的情况下快速定位每一份材料。逐条的需求对应关系见 [MANIFEST.md](MANIFEST.md)，完整分析与协作过程见 [实验报告.md](实验报告.md)。

## 顶层材料

| 材料 | 位置 | 说明 |
| :--- | :--- | :--- |
| 课程原始任务文档 | [README.md](README.md) | 学习材料与三个 Baseline 的任务/格式要求，**未改动** |
| 实验报告 | [实验报告.md](实验报告.md) | 项目分析、人机协作过程、最终产物与总结 |
| 提交清单 | [MANIFEST.md](MANIFEST.md) | 逐条列明提交要求 ↔ 证据 ↔ 结果 |
| Prompt 截图 | [docs/stage*.png](docs/) | 10 张协作过程截图（图 1–10），含阶段四的对抗性审查两屏 |
| 运行证据截图 | [docs/result-*.png](docs/) | 10 张手工终端截图（图 11–17），对应当前代码的运行结果 |
| 顶层构建入口 | [Makefile](Makefile) | `make` / `make check` / `make unit` / `make asan` 一键跑完三个 Baseline |

## 运行证据

`docs/result-01..10.png` 为**手工终端截图**，反映当前代码的验证结果（样例 3/7/8、断言 46/28/6、复杂度增长与 Welch t 检验、ASan 干净）。Prompt 截图（图 1–10）是协作过程记录，其中图 9–10 为 `reverseSharedPath` 动手前的对抗性审查（提问 + AI 回答）。

## Baseline 概览

| Baseline | 主题 | 代码 | 样例 | 构建入口 | 结果 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| baseline1 | 链表基本实现（3 个小任务） | [baseline1/src/](baseline1/src/) | [inputs](baseline1/inputs/) / [outputs](baseline1/outputs/) | `make` / `make check` / `make unit` / `make asan` | 3/3 样例 PASS，46 项断言通过 |
| baseline2 | 可变数组（4 个小任务） | [baseline2/src/](baseline2/src/) | [inputs](baseline2/inputs/) / [outputs](baseline2/outputs/) | `make` / `make check` / `make unit` / `make bench` / `make asan` | 7/7 样例 PASS，28 项断言通过 |
| baseline3 | 幽灵交叉路口（3 算法 + 2 测试） | [baseline3/src/](baseline3/src/) | [inputs](baseline3/inputs/) / [outputs](baseline3/outputs/) | `make` / `make check` / `make unit` / `make asan` | 8/8 样例 PASS，6 项断言通过 |

统一构建参数：`-std=c++17 -Wall -Wextra`（baseline2 另加 `-O2`），三个 Baseline 实测**无警告**。每个 Baseline 均提供说明文档 [baseline1/README.md](baseline1/README.md)、[baseline2/README.md](baseline2/README.md)、[baseline3/README.md](baseline3/README.md)。

## 证据索引（截图）

| 图 | 文件 | 内容 |
| :--- | :--- | :--- |
| 图 1 | [docs/stage1-prompt-01-style-structure.png](docs/stage1-prompt-01-style-structure.png) | 提示词：明确 baseline1 的风格与结构约束 |
| 图 2 | [docs/stage1-prompt-02-continue-baseline1.png](docs/stage1-prompt-02-continue-baseline1.png) | 提示词：在现有框架上继续完成 baseline1 |
| 图 3 | [docs/stage2-prompt-01-merge-docs.png](docs/stage2-prompt-01-merge-docs.png) | 提示词：合并两份说明文档并重整目录 |
| 图 4 | [docs/stage2-prompt-02-unify-src-naming.png](docs/stage2-prompt-02-unify-src-naming.png) | 提示词：统一三个 baseline 的 src 命名 |
| 图 5 | [docs/stage3-prompt-01-start-baseline2.png](docs/stage3-prompt-01-start-baseline2.png) | 提示词：启动 baseline2 |
| 图 6 | [docs/stage3-prompt-02-verify-baseline2.png](docs/stage3-prompt-02-verify-baseline2.png) | 提示词：核对 baseline2 是否符合提交要求 |
| 图 7 | [docs/stage3-prompt-03-start-baseline3.png](docs/stage3-prompt-03-start-baseline3.png) | 提示词：启动 baseline3 |
| 图 8 | [docs/stage3-prompt-04-verify-baseline3.png](docs/stage3-prompt-04-verify-baseline3.png) | 提示词：核对 baseline3 是否符合提交要求 |
| 图 9 | [docs/stage4-prompt-01-adversarial-review-1of2.png](docs/stage4-prompt-01-adversarial-review-1of2.png) | 提示词：对 `reverseSharedPath` 拟定方案做对抗性审查（只推演指针、不写代码） |
| 图 10 | [docs/stage4-prompt-01-adversarial-review-2of2.png](docs/stage4-prompt-01-adversarial-review-2of2.png) | AI 审查回答：推演出 `9->next = 9` 自环并给出正确顺序 |
| 图 11 | [docs/result-01-make-check-all.png](docs/result-01-make-check-all.png) | 三个 baseline 的 `make check` 全部通过 |
| 图 12 | [docs/result-02-baseline1-task1-unit.png](docs/result-02-baseline1-task1-unit.png) | baseline1 小任务 1 单元验证输出 |
| 图 13 | [docs/result-03-baseline1-task2-unit.png](docs/result-03-baseline1-task2-unit.png) | baseline1 小任务 2 单元验证输出 |
| 图 14 | [docs/result-04-baseline2-extend-unit.png](docs/result-04-baseline2-extend-unit.png) | baseline2 扩展功能自测与性能统计 |
| 图 15 | [docs/result-05-baseline3-manual-test.png](docs/result-05-baseline3-manual-test.png) | baseline3 手动构造测试可视化输出 |
| 图 16 | [docs/result-06-growth-stats.png](docs/result-06-growth-stats.png) | 复杂度增长实测（`growth_*`）与 Welch t 检验（`STAT`） |
| 图 17a–d | [docs/result-07-asan-1of4.png](docs/result-07-asan-1of4.png)、[2of4](docs/result-08-asan-2of4.png)、[3of4](docs/result-09-asan-3of4.png)、[4of4](docs/result-10-asan-4of4.png) | `make asan` 四屏：ASan/LSan 干净退出 |