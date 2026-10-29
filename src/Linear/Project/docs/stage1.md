# 阶段 1 · 设计与核心实现记录

日期：2026-10-20。对应 TASKS.md 阶段 1（T1.1–T1.5）。

## 产出

| 项 | 内容 |
| --- | --- |
| 设计文档 | [design.md](design.md)：数据结构选型论证、消除/连锁算法、撤销算法（多区间逆序回填）、道具设计 |
| IO 协议 | [PROTOCOL.md](PROTOCOL.md)：driver / test / oracle / outputs 四方契约（v1.0 定稿） |
| core 实现 | [track.hpp](../src/core/track.hpp)/[track.cpp](../src/core/track.cpp)（双向链表轨道）、[waiting_queue.hpp](../src/core/waiting_queue.hpp)（环形队列模板）、[game.hpp](../src/core/game.hpp)/[game.cpp](../src/core/game.cpp)（规则引擎） |
| 自测 | [main.cpp](../src/main.cpp)：20 场景单元自测驱动（README 示例、连锁、撤销回滚、道具全覆盖） |

## 设计过程中的关键决策（超出 D1–D6 的细化）

1. **README 消除规则解读为状态规则**：≥3 同色即消，不限于"发射触发"。
   因此装载时消除既有段（README 示例 `RRBBBRR` 正是装载连锁归空），
   REVERSE 拼出的三连同样消除。这条解读已实现并测试。
2. **消除算法锚定不变量 U2**（轨道无 ≥3 同色段）：任何改轨操作后
   从变更点局部 settle 即可，无需全轨扫描。连锁的"拼缝 probe"技巧
   （删段后取右段首为下个检查点）是算法核心，见 design.md §3.2。
3. **撤销的多区间逆序回填**：连锁消除各轮区间基于当时轨道，撤销按
   发生逆序回填恰好坐标对齐（归纳证明见 design.md §5.2）。发射球
   参与消除与未参与两情形统一为"删 pos 处球"，等价性由差分测试验证。
4. **道具全部有 core 接口且实现完毕**（D4 全选的承诺在阶段 1 落地）：
   炸弹（settleFrom 首轮特例）、变色（shoot 定色参数）、SWAP（队列
   O(1) 交换）、HINT（副本模拟）、REVERSE（三指针逆置 + 双拼缝 settle）。

## 发现并修复的缺陷

**REVERSE 双拼缝 probe 越界 / use-after-free**（差分测试发现，单测
20 场景未覆盖）：

- 触发：反转 [i,j] 后两个拼缝都要 settle；第一个 settle 的连锁把
  轨道删短/删空，第二个 probe 按下标 `at(i-1)` 取 → 越界异常。
- 修复 v1 引入更隐蔽的 bug：先取指针再校验"是否仍在轨道"，但校验
  前解引用了可能已释放的节点（`probeB->ball`）→ ASan 报 UAF。
- 修复 v2（最终）：`nodeAlive()` 只比较指针值不解引用；被删则跳过
  第二次 settle（该段已被第一次消除覆盖）。

## 验证链

```sh
make && ./zuma                  # 20 场景全 PASS
# ASan/UBSan：同套源码 -fsanitize=address,undefined 重编运行 → 全 PASS
# 随机差分（未入库，报告引用数据）：10^5 局随机操作 + 撤销到底
#   → 状态精确恢复，0 失败（含 ASan 版）
```

随机差分测试程序（含 UAF 修复前后的对照）在阶段 4 将作为
`tools/` 的一部分正式入库（oracle 旁边），本阶段验证结论已写入
design.md §7。

## 阶段 1 验收（TASKS.md 勾选对应）

- [x] T1.1 数据结构详细设计（design.md §2，选型理由与复杂度表）
- [x] T1.2 模块划分（core/driver 分层，design.md §1——driver 待阶段 2）
- [x] T1.3 IO 协议固化（PROTOCOL.md v1.0）
- [x] T1.4 消除算法设计（design.md §3——且已实现 + 测试）
- [x] T1.5 撤销设计（design.md §5——且已实现 + 差分验证）

附注：T2.1–T2.5 的 core 层实现在阶段 1 已顺带完成（自测通过），
阶段 2 聚焦 driver + 渲染层 + 协议集成。
