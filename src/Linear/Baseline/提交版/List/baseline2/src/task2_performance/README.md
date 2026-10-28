# 小任务 2：可变数组与 std::vector 的性能评估

`main_performance.cpp` 编译为 `vector_perf`，在多种场景下对比自实现 `dsaac::Vector<int>` 与 `std::vector<int>` 的耗时。

## 场景设计

| 场景 | 操作 | 考察点 |
| --- | --- | --- |
| `append` | 连续尾部追加 n 次 | 扩容策略与内存搬移开销 |
| `append_reserve` | 先 `reserve(n)` 再追加 n 次 | `reserve` 对扩容次数的影响 |
| `access` | 建表后随机访问 n 次 | 随机访问 $O(1)$ 特性 |
| `insert_front` | 头部插入 m 次 | 最坏情况：每次搬移整段 $O(n)$ |
| `erase_front` | 头部删除 m 次 | 同上 |

- 每项测试重复 `REPEAT=5` 次取平均，降低计时抖动。
- 规模：`append` / `access` 用 10 万与 100 万；`insert_front` / `erase_front` 因 $O(n^2)$ 取 1/100 规模。
- 输出 `PERF` 统计行，并在末尾给出各场景耗时比值（自实现 / std::vector）。

## 验证方式

```sh
make && ./vector_perf
```

结果与差异分析见 `baseline2/README.md`。