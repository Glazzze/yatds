# 小任务 1：链表的基本实现

`list.hpp` 声明并导出教程风格的链表节点 `ListNode` 与基本操作；`list.cpp` 提供完整实现；`test_list.cpp` 为测试程序。

## 需要完成的内容

- 节点定义与链表初始化（`build_list`）。
- 教程要求的 `insert`、`remove`、`access`、`find`。
- 扩展操作：`push_front`、`push_back`、`length`、`contains`、`reverse`、`to_vector`、`free_memory`。
- 测试程序覆盖边界情况（空表、单节点、越界访问），并以交叉验证体现函数特性。

## 行为要求

- `remove` 负责释放被删节点内存，`free_memory` 负责释放整表，避免内存泄漏。
- `access` 越界返回 `nullptr`；`find` 不存在返回 `-1`。

## 验证方式

```sh
g++ -std=c++17 -Wall list.cpp test_list.cpp -o /tmp/test_list
/tmp/test_list
```

按提交规范编译运行（在含 `Makefile` 的 `baseline1/` 目录执行 `make unit` 运行本单元验证程序，或 `make && make check` 运行命令式样例比对）。

## 接口对照

| 函数 | 语义 | 时间复杂度 |
| --- | --- | --- |
| `insert(n0, P)` | 在 `n0` 之后插入节点 `P` | $O(1)$ |
| `remove(n0)` | 删除 `n0` 之后的首个节点 | $O(1)$ |
| `access(head, i)` | 访问下标 `i` 的节点 | $O(n)$ |
| `find(head, x)` | 查找首个值为 `x` 的下标 | $O(n)$ |
| `reverse(head)` | 就地反转链表 | $O(n)$，额外 $O(1)$ |
