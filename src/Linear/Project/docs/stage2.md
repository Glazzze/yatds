# 阶段 2 · driver 实现记录

日期：2026-10-20（阶段 2 后半，T2.6/T2.7）。core 层已在阶段 1 完成（见
[stage1.md](stage1.md)），本阶段交付协议 driver 与两级测试。

## 产出

| 项 | 内容 |
| --- | --- |
| [src/driver/protocol_io.hpp](../src/driver/protocol_io.hpp)/[.cpp](../src/driver/protocol_io.cpp) | 协议 IO：头部解析（COLORS/TRACK/QUEUE/LIMIT/READY + 公共校验）、指令行解析（Command 状态机，语法错误不抛异常而是产出 kCmdInvalid）、事件输出（printElim/printFinal 纯文本形态） |
| [src/main.cpp](../src/main.cpp) | driver 主循环：读指令 → 调 core → 按协议输出。终局后仅 UNDO 可执行（可回退）；QUIT 后忽略剩余指令；EOF 视同退出 |
| [tools/core_selftest.cpp](../tools/core_selftest.cpp) | 阶段 1 的 20 场景自测从 main.cpp 迁入，独立构建（`make selftest`） |
| [tools/driver_test.cpp](../tools/driver_test.cpp) | driver 集成测试：15 用例内嵌（输入， 期望输出），子进程跑 zuma 逐字节比对（`make drivertest`）；驱动路径按 `/proc/self/exe` 推算，不依赖 cwd |

## 协议演进（v1.0 → v1.0.1，实现驱动修订）

集成测试暴露三处协议歧义，已同步修订 [PROTOCOL.md](PROTOCOL.md)：

1. **SHOOT/REVERSE 无消除时的 TRACK 行**：原协议"无消除时仅回显+TRACK+QUEUE"
   表述含糊（有消除时每轮 ELIM 后各一条 TRACK）——明确为**每条指令至少
   一条 TRACK 终态行**（有消除时由最后一轮 ELIM 的 TRACK 承担）。
2. **HINT 双行问题**：原设计"回显 HINT + 结果 HINT <pos>"会输出 `HINT`
   两行——修订为 **HINT 行即结果行**（回显与结果合并）。
3. **ElimEvent 补 trackAfter 字段**：`shoot()`/`reverse()` 一次性返回全部
   事件，driver 逐轮输出时**中间轨道态**无法事后取（消除全部完成后
   trackString() 只剩终态）——core 的 ElimEvent 携带每轮消除后快照。
   这是"事件流 vs 状态查询"分层的结构性问题，早发现早改。

## 实现过程中发现并修复的缺陷

1. **ElimEvent.balls 为空（move-after-use）**：`eliminateRun` 先
   `undoEntry.removed.push_back(std::move(run))` 再用 `run.balls` 构造
   `ev.balls`——move 后源对象已空。20 场景单测与 10⁵ 差分都没抓到
   （都不断言 balls 字符串），协议输出第一次真正消费它即暴露。
   修复：先构造 ev 再 move。**教训**：字段级断言缺失时，"全绿"可能
   只覆盖了字段子集——协议消费方（driver）天然是最强的字段级测试。
2. **头部 EOF 误报**：`readHeader` 把"三行头齐全 + 立即 EOF"判为
   "头部不完整"——合法输入（装载即终局、无指令）被拒。修复：EOF 时
   phase ≥ kLimitOrReady 即合法。
3. **UNDO-after-final 主循环截断**：终局 `finished = true` 直接终止
   循环，UNDO 无法从 WIN/LOSE 回退（协议明确允许）。重构：终局打印
   终局行但循环继续，仅 UNDO 分支在终局态执行；UNDO 成功复位
   finalShown（再次终局时重新打印终局行）。

## 验证链（全部通过）

```sh
make                    # 零警告（-Wall -Wextra）
make selftest           # core 20 场景
make drivertest         # driver 15 用例（子进程逐字节比对）
# ASan/UBSan 版 zuma 主流程（undo-replay 用例）通过
```

## 阶段 2 收口

- [x] T2.6 主循环 driver（协议解析 + 输出 + 终局/UNDO 交互语义）
- [x] T2.7 driver 集成自测（15 用例：README 示例、消除回滚重放、变色
      定色/未定色、炸弹、SWAP 不足、REVERSE+撤销、HINT、LIMIT、非法
      指令全表、越界、EOF、注释、终局后防御路径）
- 备注：LIMIT×UNDO 交互已含在 reverse/undo 用例路径中（UNDO 复位
  moves 后 LIMIT 重判，core 单测场景 15 覆盖）
