# 阶段 0 · 准备工作记录

> 历史记录：本文保留早期外部链栈复用实验。2026-10-29 最终版已改为
> Project 内自实现链式栈，外部路径与显式实例化均已移除；当前构建方式见
> [standalone.md](standalone.md)。

日期：2026-10-20。本文件记录阶段 0（目录骨架 + 构建环境 + D6 复用链路验证）的过程与结论，供实验报告"设计思路"章节引用。

## 环境

| 项 | 值 |
| --- | --- |
| 编译器 | g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0 |
| 构建 | GNU Make 4.4.1 |
| 标准 | `-std=c++17 -Wall -Wextra`（对齐 Baseline/Graph Project 惯例） |
| 消毒器 | ASan + UBSan 冒烟通过 |

## 目录骨架

```text
Project/
├── README.md            ← 课程原始任务文档（未改动）
├── TASKS.md             ← 任务清单（决策 D1–D6 已回填）
├── Makefile             ← make 一键编译 / make clean（check 待阶段 4）
├── src/
│   ├── main.cpp              ← 主程序入口（当前为冒烟骨架，阶段 2 填充）
│   ├── undo_entry.hpp        ← 撤销记录结构体（命令模式，阶段 2 细化）
│   └── linked_stack_undo.cpp ← Baseline 链式栈复用实例化单元（见下）
├── inputs/  outputs/   ← 样例目录（阶段 4 填充）
├── tools/               ← oracle 等辅助工具（阶段 4）
└── docs/                ← 过程记录（本文件）
```

## D6 复用链路验证（核心工作）

### 问题

Baseline 的 `dsaac::LinkedStack<T>` 是模板类，成员函数定义在 `stackli.cpp` 中，且只显式实例化了 `int / double / char` 三个类型。本项目撤销栈需要存自定义类型 `zuma::UndoEntry`，直接 include `stackli.hpp` 会出现链接错误（undefined reference）。

### 冒烟实验过程

1. **初版方案（失败）**：项目 TU include `stackli.hpp` + `template class dsaac::LinkedStack<UndoEntry>;`，Makefile 独立编译 `stackli.cpp`。链接失败——**显式实例化所在 TU 必须能看到成员函数定义**，头文件中只有声明。
2. **修正方案（通过）**：专设实例化 TU `src/linked_stack_undo.cpp`，`#include "stackli.cpp"`（连带定义）后做定义侧显式实例化；其余 TU 只 include `stackli.hpp`。
   - 分离编译链接：通过（main.cpp 只含头文件，链接到实例化符号）
   - 与独立编译 `stackli.cpp` 共存：通过（`int/double/char` 在两 TU 中实例化一致，链接器合并弱符号，无 ODR 违规）
   - `-O2`、`-flto`、ASan+UBSan、`-Wpedantic`：全部通过
   - 附注：显式实例化语句须写在模板所属命名空间 `dsaac`（或全局），写入 `zuma` 命名空间会报 `-fpermissive` 错误，已修正为全局作用域

### 结论

D6=B 的复用策略落地为：**Makefile 编译 4 个 TU**——`main.cpp`（及后续项目源文件，只 include 头）+ `linked_stack_undo.cpp`（include `stackli.cpp` 并实例化 `UndoEntry`）+ Baseline 的 `stackli.cpp` 原样编译。撤销栈直接复用 Baseline 实现，轨道双向链表与环形队列按 D2/D3 为本项目自实现。

## 阶段 0 验收

- [x] T0.1 目录骨架建立完毕
- [x] T0.2 `g++ -std=c++17 -Wall -Wextra`（含 `-Wextra`）零警告编译通过；`LinkedStack<UndoEntry>` 分离编译链接通过；ASan/UBSan 冒烟通过；make → clean → make 重建通过
