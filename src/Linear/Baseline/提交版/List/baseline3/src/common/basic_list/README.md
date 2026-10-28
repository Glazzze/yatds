# 链表节点层（复用副本）

本目录的 `list.hpp` / `list.cpp` 是 [baseline1 小任务 1](../../../baseline1/src/task1_basic_list/) 的裸节点链表实现的**原样副本**，作为 Baseline 任务 3 三个进阶算法的底层结点与工具函数。

## 提供的接口（`namespace dsaac`，并 `using` 暴露到全局）

| 函数 | 语义 |
| --- | --- |
| `struct ListNode { int val; ListNode *next; }` | 单链表结点 |
| `build_list(values)` | 按序列建表，返回头结点 |
| `access(head, i)` | 访问下标 `i` 的结点，越界返回 `nullptr` |
| `length(head)` | 表长 |
| `reverse(head)` | 就地反转整条链表，O(n)/O(1)（`reverseSharedPath` 复用） |
| `to_vector(head)` | 导出元素序列（仅用于打印） |
| `free_memory(head)` | 释放整条链表 |

## 为什么复制而非跨目录引用

baseline1 与 baseline3 是两次独立提交，各自自包含，便于分别打包评测；`list.hpp`/`list.cpp` 与 baseline1 逐字节一致，未作修改（保持约定的 `/* START */ … /* END */` 标注与中文注释）。

## 本任务为何需要额外的构造辅助

`build_list` 只能建一条普通的无环无共享链表，而本任务的测试需要「成环」「两链表共享同一段尾部」等结构——这些无法由 `build_list` 表达，故在 [../task1_algorithms/ghost_intersection.hpp](../task1_algorithms/) 中新增了 `build_cycle` 与 `build_shared` 两个构造辅助（详见该目录 README）。