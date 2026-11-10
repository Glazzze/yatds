# 截图清单（待补）

本目录预留实验报告所需的 prompt 与运行截图位置。代码、样例、报告正文均已就绪，以下截图需在终端 / 对话界面手工截取后按文件名放入 `docs/`。报告中的图位编号与本表一致。

## 一、Prompt 与协作过程截图（6 轮）

协作按 6 轮推进：前三轮把需求和边界问清楚，第四轮确认执行顺序，第五、六轮各做一次方向修正（性能基准公平性、正确性与交付一致性）。第 5、6 轮对应实现过程中已经执行的审查动作，可按下面的 prompt 原样补发一次，以形成完整的对话与截图记录。

### 总览

| 轮次 | 图号 | 建议文件名 | 目的 | 方向确认 / 修改 |
| :---: | :---: | :--- | :--- | :--- |
| 1 | 图 1 | `stage1-prompt-01-outline.png` | 需求梳理 | 确认“先梳理、后实施” |
| 2 | 图 2 | `stage1-prompt-02-directory.png` | 目录组织 | 确认仿照 List 惯例 |
| 3 | 图 3 | `stage1-prompt-03-scope.png` | 交付范围 | 确认代码 + 样例 + 报告 |
| 4 | 图 4 | `stage1-prompt-04-plan-approval.png` | 计划批准 | 确认实施顺序与验证门槛 |
| 5 | 图 5 | `stage1-prompt-05-bench-fairness.png` | 性能基准审查 | **修改**：排除临时对象 / 未开优化，增加 LTO 对照 |
| 6 | 图 6 | `stage1-prompt-06-correctness-docs.png` | 正确性与交付审查 | **修改**：拆分两套 `nextval`，增加独立 oracle，统一文档 |

### 第 1 轮：需求梳理（图 1，`stage1-prompt-01-outline.png`）

**Prompt 原文**：

```text
src/Linear/Baseline/String/README.md根据字符串任务的实验任务文档，梳理一下完成这个任务需要推进哪些工作，等我确认后再开始工作
```

**预期 AI 动作 / 必须入镜的回复内容**：

- 判断任务范围：三个 Baseline 分别是“字符串基本实现”“模式匹配”“DNA 蛋白编码片段检索”；
- 每个 Baseline 列出 2–3 条工作要点：
  - Baseline 1：`MyString` 动态数组、≥5 个扩展接口、百万次拼接与扩容策略对比；
  - Baseline 2：朴素 → KMP → `nextval` 三步递进、极端用例与字符比较次数统计；
  - Baseline 3：6 个阅读框 + 反向互补、KMP、坐标映射、前缀和区间查询；
- 初步列出提交物：`inputs/`、`outputs/`、名为 `test` 的测试程序、`.cpp`、`Makefile`；
- 结尾明确“等确认后再开始”。

**方向确认点**：这一轮不写代码，只确认工作拆解是否完整。

**画面要求**：用户消息完整入镜；AI 回复至少保留三个 Baseline 的编号列表和结尾。若一屏放不下，可拆成 `stage1-prompt-01a-question.png`、`stage1-prompt-01b-outline.png`，报告仍标为“图 1”。

**建议图注**：*图 1　首轮需求：先依据课程文档梳理工作项，确认后再实施*

### 第 2 轮：目录组织确认（图 2，`stage1-prompt-02-directory.png`）

**Prompt 原文**：

```text
【目录组织确认】字符串实验的提交目录希望怎么组织？
选择：仿照 List 惯例——保留课程原始 README.md 和 src/ 骨架不动，新建 baseline1/、baseline2/、baseline3/ 三个自包含提交目录，再加顶层 Makefile、INDEX.md、MANIFEST.md、实验报告。
请按这个选择更新计划，先不要开始编码。
```

**预期 AI 动作 / 必须入镜的回复内容**：

- 给出 `String/` 的目标目录树：`baseline1/`、`baseline2/`、`baseline3/` 各自包含 `README.md`、`Makefile`、`inputs/`、`outputs/`、`src/`；
- 顶层补 `Makefile`、`INDEX.md`、`MANIFEST.md`、`实验报告.md`；
- 明确课程 `README.md` 与 `src/` 骨架**不修改**，每个 baseline 可单独 `make`；
- 说明三个 baseline 不共享源码、各自自包含。

**方向确认点**：目录结构从“按任务平铺”改为“按 List 惯例的三个自包含提交目录”，并确认不动课程原始文件。

**画面要求**：问题与选项（含 `Recommended` 标记）、选中状态、AI 更新的目录树同时入镜。若界面不能同屏，可拆 `stage1-prompt-02a-choice.png`、`stage1-prompt-02b-tree.png`。

**建议图注**：*图 2　目录组织确认：保留课程原文，新建三个自包含的 baseline 提交目录*

### 第 3 轮：交付范围确认（图 3，`stage1-prompt-03-scope.png`）

**Prompt 原文**：

```text
【交付范围确认】这次要交付到什么程度？
选择：代码 + 样例 + 报告——每个 baseline 都要有实现源码、inputs/outputs、名为 test 的测试程序、Makefile、README，再加完整实验报告；prompt 和终端截图我后续补，报告中预留图位和截图清单。
请把交付清单和验收命令补进计划。
```

**预期 AI 动作 / 必须入镜的回复内容**：

- 每个 baseline 的交付清单：源码、样例（含官方样例）、同名 `outputs/`、`test`、`Makefile`、`README.md`；
- 顶层交付：`INDEX.md`、`MANIFEST.md`、`实验报告.md`、`docs/` 截图清单；
- 验收命令：`make all` / `make check` / `make unit` / `make bench` / `make asan`；
- 明确截图由用户后续补入，报告预留图位。

**方向确认点**：交付范围从“只写代码”扩展为“代码 + 样例 + 文档 + 报告”，并确认截图不由 AI 生成。

**画面要求**：问题、选中项、交付清单和验收命令必须可见；这是报告中“最终产物”一节的主要依据。

**建议图注**：*图 3　交付范围确认：代码、样例、测试程序、构建文件与完整报告一并交付*

### 第 4 轮：计划批准（图 4，`stage1-prompt-04-plan-approval.png`）

**Prompt 原文**：

```text
【计划批准】按当前计划开始实施。顺序固定为 baseline1 → baseline2 → baseline3；每完成一个 baseline，先跑 make check、make unit、make asan，全部通过再进入下一个；三个都完成后再补顶层 Makefile、INDEX.md、MANIFEST.md 和实验报告。
```

**预期 AI 动作 / 必须入镜的回复内容**：

- 确认按 baseline1 → baseline2 → baseline3 推进；
- 每个 baseline 完成后执行 `make check`、`make unit`、`make asan` 并报告结果；
- 最后补顶层入口、索引、提交清单与实验报告；
- 计划中的验证步骤和“风险与已定口径”小节仍保留。

**方向确认点**：确认实施顺序与“每个 baseline 验证通过后才进入下一个”的质量门槛。

**画面要求**：优先截“计划批准”状态 + 验证命令块 + 风险小节；计划较长时拆 `stage1-prompt-04a-plan.png`、`stage1-prompt-04b-approve.png`，红框圈出 `make check / unit / asan`。

**建议图注**：*图 4　计划批准：按 baseline1 → 2 → 3 推进，每个阶段验证通过后再继续*

### 第 5 轮：性能基准审查（图 5，`stage1-prompt-05-bench-fairness.png`）

**Prompt 原文**：

```text
【方向修正：性能基准】baseline1 目前 MyString 比 std::string 慢 2 倍多，先不要写结论。检查两件事：① 基准里每次 append 是否构造了临时 MyString 对象，造成额外的堆分配；② test 和 bench 是否都开启了 -O2。修正后重测，再用 -O2 -flto 编译同一基准做对照，确认差距来自算法/数据结构还是跨编译单元调用开销，并把数据和判断写进 baseline1/README.md 和实验报告。
```

**预期 AI 动作 / 必须入镜的回复内容**：

- 指出 `append(MyString("x"))` 每次都会构造临时对象、产生堆分配，改为 `append("x")`；
- 给 `test` 与 `bench` 统一加 `-O2`，说明内嵌 PERF 只作格式快照；
- 用 `-O2 -flto` 重编同一基准：`MyString` 2.954 ms 反超 `std::string` 3.554 ms；
- 结论：默认构建下约 2.2 倍差距主要来自跨编译单元调用未内联，而非算法差异；更新 README 与报告。

**方向修改点**：从“直接报告 MyString 更慢”改为“先控制变量、再解释差距来源”，并新增 LTO 对照实验。

**画面要求**：prompt 与 AI 的审查结论、修正后的 `PERF` 数据、LTO 对照表至少同屏出现两项；终端数据可用图 8（`make -C baseline1 bench`）补充。

**建议图注**：*图 5　方向修正：先排除临时对象与优化等级干扰，再用 LTO 定位差距来源*

### 第 6 轮：正确性与交付审查（图 6，`stage1-prompt-06-correctness-docs.png`）

**Prompt 原文**：

```text
【方向修改：正确性与交付】baseline2 的 NEXTVAL 必须复现官方样例 AAAAB → 0 0 0 0 0，但匹配不能因此漏匹配。先手推失配路径；如果两者冲突，就拆成“课程样例口径”和“扫描安全口径”两张表，在 README、单元断言和报告中解释。baseline3 再写一个不复用 KMP 的独立暴力 oracle，随机差分重点验证反向互补坐标、重叠匹配和查询边界。最后统一检查三个 baseline 的 README、INDEX.md、MANIFEST.md、实验报告和截图清单是否与实现一致。
```

**预期 AI 动作 / 必须入镜的回复内容**：

- 手推 `AAAAB` 的失配路径，说明样例口径的 `0 0 0 0 0` 不能直接驱动扫描；
- 拆分 `getNextVal`（样例口径）与 `getNextValFallback`（扫描安全），在 README 与 35 项断言中说明；
- 新增 `baseline3/tools/oracle.py`：独立密码表、按定义算 `pi`、逐位置暴力匹配、独立坐标映射，300 轮随机差分全部一致；
- 统一核对文档与实现：17/17 样例 PASS、135 项断言、ASan 干净；
- 更新 `INDEX.md`、`MANIFEST.md`、实验报告与截图清单。

**方向修改点**：从“样例通过即可”改为“样例口径与算法正确性分离”，并为 DNA 增加独立正确性证据；最后做一次文档与实现的一致性审查。

**画面要求**：prompt、两套 `nextval` 的数组对照（`AAAAB` → `0 0 0 0 0` vs `0 0 0 0 3`）、oracle 的“300 轮全部一致”三者至少出现两项；数组对照可用图 10（`make -C baseline2 bench`）、oracle 用图 12。

**建议图注**：*图 6　方向修改：拆分样例口径与扫描口径，并用独立 oracle 验证 DNA 坐标映射*

### 截图规范

- **真实优先**：截取实际对话记录，不重新摆拍、不改写消息；如界面含时间戳、模型名，保留即可。第 5、6 轮若当时由 AI 自主审查，可按上文的 prompt 补发一次再截图，并在图注中注明“审查环节”。
- **完整性**：每条消息从气泡顶部到结尾完整入镜；长消息宁可分屏，不要截半句。
- **可读性**：窗口宽度 ≥ 1200 px，正文字号不小于 14 px；导出 PNG，不要用有损压缩。
- **标注**：可用红框标出每轮的“确认 / 修改”关键词，以及验证命令、关键数据。
- **顺序**：文件编号与 6 轮推进顺序一致，便于报告按图叙述。

## 二、运行结果截图

在 `src/Linear/Baseline/String/` 下依次执行并截图：

| 图号 | 建议文件名 | 命令 | 预期画面 |
| :---: | :--- | :--- | :--- |
| 图 7 | `result-01-b1-check-unit.png` | `make -C baseline1 check && make -C baseline1 unit` | 5/5 PASS，70 项断言通过 |
| 图 8 | `result-02-b1-bench.png` | `make -C baseline1 bench` | 连续拼接、扩容策略、百万规模 PERF 行 |
| 图 9 | `result-03-b2-check-unit.png` | `make -C baseline2 check && make -C baseline2 unit` | 6/6 PASS，35 项断言通过 |
| 图 10 | `result-04-b2-bench.png` | `make -C baseline2 bench` | 朴素 vs KMP、nextval 比较次数与 RATIO 行 |
| 图 11 | `result-05-b3-check-unit.png` | `make -C baseline3 check && make -C baseline3 unit` | 6/6 PASS，30 项断言通过 |
| 图 12 | `result-06-b3-oracle.png` | `python3 baseline3/tools/oracle.py 300 20261110` | 随机差分 300 轮全部一致 |
| 图 13 | `result-07-asan.png` | `make asan` | 三个 Baseline 的 ASan/LSan 干净退出 |
| 图 14 | `result-08-top-check.png` | `make check` | 顶层一键跑完 17 个样例 |

> 截图时请保留命令与完整输出（含末尾统计行），文件名与上表一致即可直接被报告引用。若终端字体较小，建议单屏只截一个 `make` 目标，必要时分多张（如 `result-07-asan-1of3.png` …），图号仍归为对应的图 13。