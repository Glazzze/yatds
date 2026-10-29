# 样例集设计（阶段 4 · T4.1/T4.2）

日期：2026-10-28。`inputs/caseNN.txt` 与 `outputs/caseNN.txt` 同名配对，
`make check` 遍历比对。全部样例无随机性，输出由主程序纯协议模式生成
（`make samples`），并逐例人工核对语义。

## 覆盖矩阵

| 用例 | 覆盖点 | 关键期望 |
| --- | --- | --- |
| [case01](../inputs/case01.txt) | 普通插入（中间位置，无消除） | `TRACK RBYG`，分数 0 |
| [case02](../inputs/case02.txt) | 三连消除（单轮） | `RRR` 消除 +30 |
| [case03](../inputs/case03.txt) | 多级连锁（README 示例） | `BBB`→`RRRR`→空，`WIN 110` |
| [case04](../inputs/case04.txt) | 不足三连 | 插入后 `RRBR` 仅两连，不消除 |
| [case05](../inputs/case05.txt) | 插入位置边界 `0` 与末端 `p=n` | 两次插入均合法，无消除 |
| [case06](../inputs/case06.txt) | 撤销后再消除 | `UNDO` 恢复局面后三连消除 |
| [case07](../inputs/case07.txt) | 撤销多级连锁 | `WIN` 后 `UNDO` 完整回滚到 6 球 |
| [case08](../inputs/case08.txt) | 道具 · 炸弹球 `*` | 以炸弹为中心消 `R*G`（3 球） |
| [case09](../inputs/case09.txt) | 道具 · 变色球 `?` + 失败终局 | 非法颜色零副作用 → 定色消除 → `LOSE` |
| [case10](../inputs/case10.txt) | 道具 · `SWAP` | 交换队首两球后消除；不足两球 `ERROR` |
| [case11](../inputs/case11.txt) | 道具 · `HINT` | 有解 `HINT 0`；无解 `HINT -1` |
| [case12](../inputs/case12.txt) | 道具 · `REVERSE` | 反转 `[2,5]` 拼出四连消除 |
| [case13](../inputs/case13.txt) | 失败终局 · `LIMIT` | 步数达上限，`LOSE 30 MOVES=2` |
| [case14](../inputs/case14.txt) | 失败终局 · 队列耗尽 | 轨道非空且无待发射球，`LOSE` |
| [case15](../inputs/case15.txt) | 胜利终局 | 最后一球补齐三连，`WIN 30` |
| [case16](../inputs/case16.txt) | 错误输入 | 越界发射 + 空撤销栈均 `ERROR`，`MOVES=0` |

## 设计约定

- 头部注释以 `#` 开头（协议 §7 注释行），用于说明用例意图，不参与输出。
- 需要展示"进行中局面"的用例显式 `QUIT`（输出 `QUIT_SCORE`）；胜/负终局
  用例在终局行后不再追加指令（协议 §3.3）。
- 错误输入样例（case09/10/16）同时验证"非法指令零副作用、不计步"。
- 规模小、事件链短，便于人工核对；边界与道具逐项覆盖，胜/负三条路径
  （清空、队列耗尽、步数上限）齐备。