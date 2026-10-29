# 阶段 3 · CLI 用户交互（T3.1–T3.3）

日期：2026-10-28。交付轨道 CLI 渲染、ANSI 颜色与 NO_COLOR 开关、HELP
指令与友好错误提示。协议同步升到 v1.1（见 [PROTOCOL.md](PROTOCOL.md) §8）。

## 产出

| 项 | 内容 |
| --- | --- |
| [src/driver/cli_render.hpp](../src/driver/cli_render.hpp)/[.cpp](../src/driver/cli_render.cpp) | 渲染层：命令行开关（`--ui/--plain/--color/--no-color/-h`）、球串逐球 ANSI 上色、事件提示、状态摘要、错误建议、HELP 正文 |
| [src/main.cpp](../src/main.cpp) | 接入渲染层：双模式输出、`HELP` 分支、每步状态摘要与道具/消除提示、错误后追加友好建议 |
| [src/driver/protocol_io.cpp](../src/driver/protocol_io.cpp) | 指令名大小写不敏感；新增 `HELP` 解析；`printElim` 增加可选上色钩子（纯文本内容不变） |
| [src/core/game.cpp](../src/core/game.cpp) | 修复：变色球颜色校验先于 `take()`（非法指令零副作用，见下） |
| [tools/driver_test.cpp](../tools/driver_test.cpp) | 新增 4 个用例（HELP、大小写、零副作用、交互渲染）+ 4 项颜色开关断言 |

## 3.1 双模式渲染：为什么不把提示行写进契约

协议输出（`outputs/`、`test`、oracle）是**逐字节契约**，任何提示行都会破坏
比对。因此渲染分两档、共用同一份事件数据：

| 模式 | 判定 | 内容 |
| --- | --- | --- |
| 纯协议 | stdout **非终端**（默认）或 `--plain` | PROTOCOL.md §3 原文，零附加 |
| 交互渲染 | stdout **是终端**（默认）或 `--ui` | 协议行 + 两空格开头的提示行 |

- 默认按 `isatty(stdout)` 判定：`test` 以管道/重定向运行主程序，自动落到
  纯协议模式，无需改测试代码；`--ui` 供演示与新的集成用例强制开启。
- 提示行统一前缀，便于人工阅读且不与协议 token 混淆：
  - `  ✦` 事件提示：消除/连锁/炸弹/变色/道具生效、未触发消除；
  - `  ▸ 提示：` 错误建议（ERROR 行保持不变）；
  - `  ·` 状态摘要：分数、步数、轨道剩余球数、队列剩余球数（有 LIMIT 时附上限）。
- 每步操作后：协议行给出轨道/队列的**内容**，状态摘要给出**剩余数量**，
  事件提示把 `ELIM round=N` 翻译成"连锁 ×N：再消除 k 球 +p 分"。

## 3.2 ANSI 颜色与 NO_COLOR

- 普通球：助记字母固定色（R 红 / G 绿 / B 蓝 / Y 黄 / M 洋红 / C 青 /
  W 白 / K 灰）；其他字母按 COLORS 声明顺序取未被占用的调色板槽位，
  同一局面内尽量不撞色。
- 特殊球：`*` 白字红底、`?` 亮洋红。
- 作用范围：开头回显、`TRACK`、`QUEUE`、`ELIM ... balls=` 的球串；命令名
  与标签不上色，保持输出可读。
- 开关优先级：`--color`（强制开）> `--no-color` > `NO_COLOR` 环境变量
  （存在且非空即关）> `TERM=dumb` > 默认（**stdout 是终端且交互模式**
  才开——`--ui` 重定向到文件时也不带颜色码）。
- **测试保证**：纯协议模式不带任何转义码；即使强制 `--ui`，`NO_COLOR=1`
  也得到纯文本。集成测试对四种组合（NO_COLOR / --no-color / --color 覆盖
  NO_COLOR / 纯协议）逐一断言；阶段 4 的 `test` 以 `--plain` + `NO_COLOR=1`
  运行主程序，双保险。

## 3.3 帮助与友好错误提示

- `HELP` 为元指令：不改状态、不计步、终局后仍可用；正文固定并写入
  PROTOCOL.md §3.2，两种模式内容一致（交互模式仅命令名上色）。
- 指令名大小写不敏感（`shoot 2` 等价 `SHOOT 2`），回显保留原文。
- 交互模式下，每条 `ERROR` 后追加一行 `▸ 提示：`，按原因码给出可操作建议：
  位置越界时给出 `0..n` 的合法范围与实际轨道长度；空队列、变色球用法、
  普通球误带颜色、REVERSE 区间、SWAP 不足两球、空撤销栈、终局后操作
  各有专门提示；语法错误按指令给出用法，未知指令引导到 `HELP`。

### 顺带发现并修复的缺陷（非法指令零副作用）

`Game::shoot` 原先先 `queue_.take()` 再校验变色球颜色：颜色非法/缺失时
异常已抛出，但**队首球已被吞掉**，违反协议 §3.4"非法指令不改变任何游戏
状态"。既有 15 用例只断言输出、不断言后续状态，因此未暴露。修复为先读
`queue_.front()` 校验、通过后再 `take()`；新增 `chameleon-invalid-color`
用例（非法后再发射一颗，轨道/队列/分数均正确）锁定行为。

## 验证链

```sh
make                    # -Wall -Wextra 零警告
make selftest           # core 20 场景
make drivertest         # driver 21 用例 + 4 项颜色开关断言
```

drivertest 新增覆盖：`help`（HELP 正文逐字节）、`help-after-final`（元指令
可用且终局行不重复）、`lowercase-cmd`、`chameleon-invalid-color`（零副作用）、
`ui-render`（NO_COLOR 下交互行逐字节可比对）、`ui-chain`（连锁提示）、
`color/*`（四类颜色开关组合）。手工冒烟：

```sh
printf 'COLORS RB\nTRACK RRB\nQUEUE RB\nSHOOT 2\nHINT\n' | ./zuma --ui
printf 'COLORS RB\nTRACK RRB\nQUEUE RB\nSHOOT 2\n' | ./zuma          # 纯协议
printf 'COLORS RB\nTRACK RB\nQUEUE R\nHELP\n' | NO_COLOR=1 ./zuma --ui
```

## 阶段 3 收口

- [x] T3.1 轨道 CLI 渲染：每步轨道/队列剩余、消除/连锁/炸弹/道具事件提示
- [x] T3.2 ANSI 颜色码 + NO_COLOR 关闭；纯协议模式逐字节不变（测试保证）
- [x] T3.3 HELP 指令 + 非法位置/空队列/空撤销栈等场景的友好提示
- 备注：网页交互为阶段 5 主力呈现；本阶段 CLI 渲染同时服务于演示与测试通道。