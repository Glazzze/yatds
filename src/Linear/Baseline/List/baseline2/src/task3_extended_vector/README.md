# 小任务 3：扩展功能实现与 std::vector 对比

`vector.hpp`（header-only）为扩展版可变数组（在小任务 1 基础上新增 6 个功能）；`main_extend.cpp` 编译为 `vector_extend`，对扩展功能逐项断言自测，并与 `std::vector` 做耗时对比。

## 新增的 6 个功能

| 功能 | 语义 | 复杂度 | 对照 `<vector>` |
| --- | --- | --- | --- |
| `pop_back()` | 删除尾元素（空表抛 `out_of_range`） | $O(1)$ | `std::vector::pop_back` |
| `resize(n, val)` | 改变元素个数：不足补 `val`、多余截断 | $O(n)$（扩容时搬移） | `std::vector::resize` |
| `swap(other)` | 交换两个数组内容（只换指针与长度） | $O(1)$ | `std::vector::swap` |
| `assign(count, val)` | 用 `count` 个 `val` 重新填充 | $O(n)$ | `std::vector::assign` |
| `find(x)` | 返回首个 `x` 的下标，不存在返回 `-1` | $O(n)$ | `std::vector` 无直接接口（对照 Java `ArrayList.indexOf`） |
| `shrink_to_fit()` | 将容量收缩到与长度一致，释放多余内存 | $O(n)$ | `std::vector::shrink_to_fit` |

自测程序对以上功能构造期望结果并断言（共 28 项），覆盖：各功能常规语义、`pop_back` 空表抛异常、`resize(0)` / 空表 `resize` / 空表 `shrink_to_fit` 等边界，以及**非平凡类型 `std::string`** 上的 `pop_back`/`insert`/`erase`/`shrink_to_fit`/`resize`——用于证明构造 / 析构严格配对（配合 `make asan` 无语义错误）。

## 验证方式

```sh
make && make unit      # 等价于 ./vector_extend
```

输出扩展功能自测统计（通过/失败项数）与 `vector_ext` / `std_vector_ext` 的耗时对比 `PERF` 行。差异分析见 `baseline2/README.md`。