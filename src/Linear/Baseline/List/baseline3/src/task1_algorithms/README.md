# 进阶算法模块：幽灵交叉路口

`ghost_intersection.hpp` / `ghost_intersection.cpp` 实现 Baseline 任务 3 的求解类与 RAII 场景持有者。

## 核心算法（`class GhostIntersection`，纯静态方法）

| 方法 | 语义 | 时间 | 空间 |
| --- | --- | --- | --- |
| `detectAndRemoveCycle(head)` | Floyd 快慢指针探环 → 定位环入口 → 就地解环；返回入口结点（无环返回 `nullptr`） | O(n) | O(1) |
| `getIntersectionNode(a, b)` | 双指针找两链表交汇点（比较**指针**而非值）；不相交返回 `nullptr` | O(lenA+lenB) | O(1) |
| `reverseSharedPath(a, b, inter)` | 仅反转公共尾部，各自独立前缀不被破坏；返回公共段新头 | O(n) | O(1) |

三者均不使用哈希表或额外数组记录结点，且全为迭代实现（递归会引入 O(n) 栈空间）。

### `reverseSharedPath` 的两个关键点

1. **head 按引用传入**（`ListNode *&`）：前缀为空时该链表的头结点本身就是交汇点，反转会改变头。
2. **必须在反转前捕获前缀尾结点**：反转会把交汇点的 `next` 置空并改变共享段方向；若反转后再沿前缀遍历，遍历会越过原前缀尾、直接走进已反转的共享段，重新接线时闭合成环，导致遍历与释放永久挂死。

### `getIntersectionNode` 的写法要点

必须写成 `pA = pA ? pA->next : headB;`，使比较能看到 `nullptr`。常见的 `pA = pA->next; if (!pA) pA = headB;` 变体会让比较永远看不到 `nullptr`，两链表不相交时死循环。

## RAII 场景持有者（`struct Scenario`）

统一管理一条或两条测试链表的生命周期，析构时按正确规则释放，规避两类常见错误：

- **先解环再释放**：对仍成环的链表直接 `free_memory` 会沿环无限遍历。
- **共享尾部只释放一次**：相交链表两个头都 `free_memory` 会双重释放共享尾部。故各自前缀释放到 `boundary` 为止，公共部分只 `free_memory(boundary)` 一次。
- 反转后 `boundary` 同步更新为公共段新头（`reverseShared()`），维持「按当前共享头释放一次」的不变量。

## 构造辅助

`build_cycle(values, entry)`：建表后令尾结点指回下标 `entry` 的结点（`entry < 0` 表示无环）。
`build_shared(pa, pb, shared, &headA, &headB)`：构造两条共享同一段物理尾部结点的链表，前缀可为空；返回公共段头结点。

## 验证

```sh
make unit    # 手动构造测试（main_manual.cpp）
make asan    # ASan/LSan 内存检查：零泄漏、无双重释放
```