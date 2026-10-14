# 小任务 2：链表模板类

`linked_list.hpp` 声明并导出模板类 `dsaac::LinkedList<T>`；`linked_list.cpp` 提供实现（含显式实例化）；`test_linked_list.cpp` 为测试程序。

## 需要完成的内容

- 将链表实现为模板类，支持任意数据类型的存储。
- 封装基本操作：`push_front`、`push_back`、`insert`、`erase`、`at`、`find`、`contains`、`size`、`empty`、`clear`。
- 扩展操作：`reverse`、`to_vector`、`front`、`back`，以及为任务 3 预留的裸节点入口 `head_node()`。
- 测试程序分别以 `int`、`double`、`char` 实例化模板，并覆盖深拷贝、自赋值、越界异常与尾指针一致性。

## 行为要求

- RAII 管理：析构函数释放全部节点，拷贝构造与赋值为深拷贝，避免浅拷贝共享节点。
- 维护尾指针 `tail_` 使 `push_back` 为 $O(1)$；删除头/尾节点时同步维护 `tail_`。
- 越界的 `at`/`insert`/`erase` 抛出 `std::out_of_range`，空表 `front`/`back` 同样抛出。

## 验证方式

```sh
g++ -std=c++17 -Wall linked_list.cpp test_linked_list.cpp -o /tmp/test_linked_list
/tmp/test_linked_list
```

按提交规范编译运行（在含 `Makefile` 的 `baseline1/` 目录执行 `make unit` 运行本单元验证程序，或 `make && make check` 运行命令式样例比对）。

## 接口对照

| 方法 | 语义 | 时间复杂度 |
| --- | --- | --- |
| `push_front(x)` | 头部插入 | $O(1)$ |
| `push_back(x)` | 尾部插入（维护尾指针） | $O(1)$ |
| `insert(i, x)` | 下标 `i` 处插入 | $O(n)$ |
| `erase(i)` | 删除下标 `i` 处元素并释放内存 | $O(n)$ |
| `at(i)` | 访问下标 `i`，越界抛异常 | $O(n)$ |
| `find(x)` | 查找首个 `x` 的下标 | $O(n)$ |
| `reverse()` | 就地反转 | $O(n)$，额外 $O(1)$ |
