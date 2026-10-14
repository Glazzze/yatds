# 小任务 4：可变数组与链表的性能对比

`main_compare.cpp` 编译为 `array_list_cmp`，在 4 种典型场景下对比可变数组 `dsaac::Vector<int>` 与链表 `LinkedList<int>` 的耗时。链表实现复用自 `src/common/linked_list/`（baseline1 小任务 2）。

## 场景设计

| 场景 | 操作 | 可变数组复杂度 | 链表复杂度 | 考察点 |
| --- | --- | --- | --- | --- |
| `append` | 尾部追加 n 次 | 均摊 $O(1)$ | $O(1)$（维护尾指针） | 连续内存 vs 逐节点分配的缓存差异 |
| `prepend` | 头部插入 m 次 | $O(n)$（整段搬移） | $O(1)$ | 链表在头部操作的优势 |
| `traverse` | 建表后顺序遍历 n 次 | $O(n)$ | $O(n)$ | 同为线性遍历，考察缓存局部性 |
| `random` | 建表后随机访问 k 次 | $O(1)$ | $O(n)$ | 随机访问的根本差异 |

- 每项重复 `REPEAT=5` 次取平均。
- 规模：`append` / `traverse` / `random` 用 10 万与 100 万；`prepend` 因可变数组为 $O(n^2)$ 取 1/100 规模；`random` 的访问次数 `k=2000`（链表每次访问 $O(n)$，控制总耗时）。
- 随机下标用线性同余生成器产生，避免纯顺序预取。
- 输出 `PERF` 统计行，并在末尾给出各场景耗时比值（链表 / 可变数组）。

## 验证方式

```sh
make && ./array_list_cmp
```

结果与场景分析见 `baseline2/README.md`。