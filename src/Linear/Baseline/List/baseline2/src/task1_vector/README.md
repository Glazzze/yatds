# 小任务 1：可变数组的基本实现

`vector.hpp` 声明并导出模板可变数组 `dsaac::Vector<T>`；`vector.cpp` 提供完整实现（含显式实例化）；`main_basic.cpp` 为命令式测试程序（编译为 `test`）。

## 需要完成的内容

- 连续内存存储，维护 `size` / `capacity` 双计数，容量不足时按 **2 倍**策略自动扩容。
- 基本操作：`push_back`、`insert`、`erase`、`at`、`size`、`capacity`、`empty`、`clear`。
- 扩展操作：`operator[]`、`front`、`back`、`reserve`、`to_vector`。
- RAII 内存管理：构造 / 拷贝构造 / 拷贝赋值 / 析构统一管理连续内存。
- 测试程序按课程 README 规定的输入格式读 `inputs/` 样例、执行操作序列，并与 `outputs/` 标准输出逐用例比对。

## 行为要求

- `capacity` 为初始容量，要求 `capacity >= n`；元素随 `push_back` 自动扩容。
- 容量按 2 倍增长（首次从 0 到 1）——报告中对扩容策略加以说明。
- 越界的 `at` / `insert` / `erase` 抛 `std::out_of_range`；命令式驱动对越界 `GET` / `INSERT` / `ERASE` 输出 `ERROR` 且不中断。
- 空数组 `PRINT` 输出 `EMPTY`；`CLEAR` 只清空元素、保留容量（不缩容）。

## 交叉校验设计

`test` 将同一初始元素与操作序列同时作用于自实现 `dsaac::Vector<int>` 与 `std::vector<int>`，逐行比对功能输出；**容量行因两者扩容策略不同而跳过**。这保证自实现的语义与标准库一致，同时允许容量策略不同。

## 验证方式

```sh
# 单元方式（在 baseline2/ 目录）
make && make check          # 自动遍历 inputs/ 与 outputs/ 比对

# 单用例调试
./test inputs/case01.txt
```

## 接口对照

| 方法 | 语义 | 时间复杂度 |
| --- | --- | --- |
| `push_back(x)` | 尾部追加（容量不足自动扩容） | 均摊 $O(1)$ |
| `insert(i, x)` | 下标 `i` 处插入 | $O(n)$ |
| `erase(i)` | 删除下标 `i` 处元素 | $O(n)$ |
| `at(i)` / `operator[](i)` | 随机访问下标 `i` | $O(1)$ |
| `front()` / `back()` | 首 / 尾元素 | $O(1)$ |
| `reserve(n)` | 预留容量 | $O(n)$（搬移） |
| `clear()` | 清空元素（不缩容） | $O(1)$ |