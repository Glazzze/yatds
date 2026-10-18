# 截图清单

本目录存放《栈与队列》实验报告的**提示词截图**与**运行结果截图**，与 `../../List/docs/` 的组织方式一致。报告共 13 张图，全部已就位。

## 提示词截图（图 1–6）

| 文件名 | 报告图号 | 内容 |
| :--- | :--- | :--- |
| `stage1-prompt-01-dependency-architecture.png` | 图 1 | 确认 `src/` 下依赖代码用途，并请 AI 建议文件架构 |
| `stage2-prompt-01-style-structure.png` | 图 2 | 完成 baseline1：风格对齐学习材料、加中文注释、三小任务分开 |
| `stage2-prompt-02-migrate-to-skeleton.png` | 图 3 | 把 baseline1 实现迁移合并到给定 `src/array_stack`，不改给定头文件 |
| `stage3-prompt-01-verify-all-baselines.png` | 图 4 | 整体自检：`src` 代码是否满足 3 个 baseline 任务与提交标准、有无 bug |
| `stage3-prompt-02-verify-baseline3-io.png` | 图 5 | 单独复核 Baseline 任务 3 的输入输出格式与样例 |
| `stage4-prompt-01-tradeoff.png` | 图 6 | 关键取舍（队列实现栈：单队列旋转 vs 双队列法），要求"选项 + 推荐 + 理由" |

## 运行结果截图（图 7–13）

| 文件名 | 报告图号 | 内容 |
| :--- | :--- | :--- |
| `result-06-oracle-case06-fix.png` | 图 7 | `git diff` 显示 case06 删除的空行 + `make oracle` 输出 8/8 通过 |
| `result-05-asan-ubsan.png` | 图 8 | `-fsanitize=address,undefined` 下三个程序干净退出（exit=0） |
| `result-01-make-check-baseline1.png` | 图 9（之一） | baseline1 `make check` 8/8 通过 + array/linked/std 三方 PERF |
| `result-01-make-check-baseline2.png` | 图 9（之二） | baseline2 `make check` 8/8 通过 + circular/linked/std 三方 PERF |
| `result-01-make-check-baseline3.png` | 图 9（之三） | baseline3 `make check` 4/4 通过 + 综合应用四方 PERF |
| `result-07-perf-space-1.png` | 图 10（之一） | `make perf`：baseline1/2 均值 ± 标准差 + Welch t 检验 |
| `result-07-perf-space-2.png` | 图 10（之二） | `make perf`：baseline2 两解法 / baseline3 四方 Welch t 检验 |
| `result-07-perf-space-3.png` | 图 10（之三） | `make perf` 尾部 + `make space` 的 peak-RSS 探针输出 |
| `result-02-baseline1-extend-unit.png` | 图 11 | baseline1 `make extend`：扩展栈 **62** 项断言 + PERF |
| `result-03-baseline2-methods-extend.png` | 图 12 | baseline2 `make methods` 4 项 + `make extend` **33** 项 |
| `result-04-baseline3-scenarios.png` | 图 13 | baseline3 `./test` 四场景（含 `CONVERT` 双向）运行输出 |