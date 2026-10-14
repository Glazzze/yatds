# 链表实现（复用副本）

本目录的 `linked_list.hpp` / `linked_list.cpp` 是 [baseline1 小任务 2](../../../baseline1/src/task2_linked_list/) 的 `LinkedList<T>` 模板链表类的**原样副本**，供 Baseline 任务 2 小任务 4（可变数组 vs 链表性能对比）使用。

## 为什么复制而非共享

- 任务 4 需要链表作为对照组，直接复用任务 1 已验证的实现，避免重复造轮子。
- baseline1 与 baseline2 是两次独立提交，各自自包含，便于分别打包与评测；因此以副本形式内联，而非跨目录引用。

## 提供的接口（小任务 4 用到的部分）

- `push_back` / `push_front`：尾插 $O(1)$（维护 `tail_`）/ 头插 $O(1)$
- `at(i)`：下标访问，链表为 $O(n)$
- `head_node()`：返回头节点裸指针，供顺序遍历基准直接沿 `next` 走，避免逐次 `at` 的 $O(n)$ 放大

完整接口与实现说明见 baseline1 的 [src/task2_linked_list/README.md](../../../baseline1/src/task2_linked_list/README.md)。