# 栈和队列 · Baseline 任务 2（队列的基本实现）提交说明

本目录（`baseline2/`）完成课程 README 中 Baseline 任务 2 的四个小任务。课程 [README](../README.md) 为原始任务文档，不做改动。依赖代码复用 `../src/queue/queue.cpp`（给定的 `CircularQueue` 实现）。

## 目录结构

```text
StackAndQueue/
├── README.md              ← 课程原始任务文档（未改动）
├── src/queue/             ← queue.hpp(给定) + queue.cpp(实现)
└── baseline2/             ← 本提交目录（Baseline 任务 2）
    ├── Makefile           ← make 生成 test / queue_methods / queue_extend
    ├── README.md          ← 本提交说明
    ├── src/
    │   ├── main_test.cpp        ← 小任务 1/3：循环 vs 链式队列交叉校验 + 三行 PERF（编译为 test）
    │   ├── linked_queue.hpp/.cpp  ← 链式队列（新增，给定骨架未提供）
    │   ├── sized_queue.hpp/.cpp   ← 第二种解法：front + size 计数法
    │   ├── main_methods.cpp       ← 小任务 2：两种解法对照（编译为 queue_methods）
    │   ├── extended_queue.hpp/.cpp ← 小任务 4：扩展队列五功能
    │   └── main_extend.cpp        ← 小任务 4：扩展自测 + 与 std::queue 对比（编译为 queue_extend）
    ├── inputs/            ← 自测样例（case01 ~ case08）
    └── outputs/
```

## 小任务划分

| 小任务 | 文件 | 内容 |
| --- | --- | --- |
| 1 | `main_test.cpp` | 循环队列 + 链式队列两种实现，命令式测试并**交叉校验** |
| 2 | `sized_queue.*`、`main_methods.cpp` | 两种"front/rear 到尾部"解决方案的对照 |
| 3 | `main_test.cpp`（PERF 部分） | 循环 / 链式 / `std::queue` 性能差异 |
| 4 | `extended_queue.*`、`main_extend.cpp` | 参照 `<queue>` 扩展 5 功能 + 与 `std::queue` 对比 |

## 构建与自测

```sh
make           # 生成 test / queue_methods / queue_extend
make check     # 样例比对
make methods   # 两种解法对照
make extend    # 扩展功能自测
```

## 小任务 2：两种解决方案的详细说明（重点）

数组中删除首元素是 O(n)，故数组队列不能靠搬移实现出队。当 `front` 和 `rear` 不断右移、到达数组尾部时无法继续——下面两种方案都让下标**回绕**到头部，把数组"首尾相接"成环，但**区分空/满**的手段不同：

**方案 A —— 空槽法（给定骨架 `CircularQueue`）**
- 构造时分配 `capacity + 1` 个槽，**故意闲置一个槽永不存数据**。
- 判空：`front == back`（两者相等即没有元素）。
- 判满：`(back + 1) % size == front`（队尾的下一格回绕后撞上队首，说明只差一格就满）。
- 入队写 `buffer[back]` 后 `back = (back + 1) % size`；出队仅 `front = (front + 1) % size`。
- 特点：判据只依赖两个下标，无需额外状态位；代价是内部多占 1 个槽的存储。

**方案 B —— size 计数法（本目录 `SizedQueue`，取自父级 README 正文思路）**
- 分配恰好 `capacity` 个槽（**无浪费**），额外维护 `front` 下标与 `size` 计数，令 `rear = (front + size) % capacity`。
- 判空：`size == 0`；判满：`size == capacity`。
- 入队写 `buffer[(front + size) % capacity]` 后 `size++`；出队 `front = (front + 1) % capacity; size--`。
- 特点：存储无浪费、容量利用率 100%；代价是需维护一个计数变量（与两个下标同步）。

`queue_methods` 对两方案跑同一序列（含回绕）交叉校验行为一致，并显示：**给定 capacity 下两者外部容量相同（都能存满 capacity 个）**，区别在内部策略——A 多分配 1 槽、判据更简；B 精确分配、需计数。两者时间均为 O(1)。

## 小任务 1/3：程序与结果分析

**实现要点**：
- **循环队列**（给定）：环形缓冲 + 空槽法，见方案 A。
- **链式队列**（`LinkedQueue`）：头节点为队首、尾节点为队尾；`push` 尾插、`pop` 头删均 O(1)，析构逐个 `delete` 防泄漏。链表按需分配，**无容量上限**。
- **交叉校验**：`test` 把同一序列在两实现上重放、逐行比对，不一致即报错。驱动层统一以输入 `capacity` 对两队列限流，使输出可比。
- **受限接口下的 SIZE/PRINT**：接口无 `size()`、无遍历。驱动用**倒队技巧**——逐个出队到 `vector`（天然得到队首→队尾序），计数/打印后按同序压回（FIFO 序不变）。

**结果**：`make check` case01~case08 全部 PASS，两实现输出完全一致。样例覆盖正常（case01）、空队列（case02/04）、满队列（case03）、弹空复用（case04）、**环形回绕**（case05 使 `back_` 越过缓冲区末尾回绕到 0）、中间态（case06）、容量复用边界（case07/08）。

**性能差异**（每项 3 次取平均，`-O2 -flto`）：

| 实现 | n=100000 | n=1000000 |
| --- | --- | --- |
| `circular_queue` | 1.84 ms | 14.21 ms |
| `linked_queue` | 4.64 ms | 37.80 ms |
| `std_queue` | 0.21 ms | 2.10 ms |

**分析**：循环队列以连续数组存储，入队/出队只写/读一个槽并推进下标，缓存友好；链式队列每次 `push` 要 `new` 节点、`pop` 要 `delete`，节点分散、缓存局部性差，且多一个指针与堆元数据的空间开销，故慢约 2~3 倍。`std::queue` 默认底层 `deque`（分段连续，无逐节点分配），三者中最快，原因与任务 1 中 `std::stack` 相同。

## 小任务 4：扩展功能与 `<queue>` 对比

因给定 `queue.hpp` 不可修改，扩展功能以独立模板类 `dsaac::ExtendedQueue<T>` 实现（自带循环缓冲存储）：

| 功能 | 语义 | 对照 `<queue>` |
| --- | --- | --- |
| `size()` | 队列长度 | `std::queue::size` |
| `clear()` | 清空全部元素 | `std::queue` 无直接 `clear` |
| `swap(other)` | 交换两队列内容 | `std::queue::swap` |
| `back()` | 访问队尾元素 | `std::queue::back` |
| `search(v)` | 元素距队首位置（队首为 1），未找到 -1 | `std::queue` 无查找接口 |

`make extend` 自测 **12 项全部通过**。与 `std::queue` 性能对比（同负载）：

| 实现 | n=100000 | n=1000000 |
| --- | --- | --- |
| `circular_queue_ext` | 1.03 ms | 9.83 ms |
| `std_queue` | 0.16 ms | 1.45 ms |

**分析**：`std::queue` 快约 1 个数量级，同样是适配器零开销 + `deque` 分段连续之故。`std::queue` 接口刻意精简，不含 `clear/search`——适配器只暴露"队首/队尾操作"的受限视图，任意位置访问（如 `search`）留给底层容器完成。

## 验证记录

- `make`：三个程序编译通过，`-Wall` 无警告。
- `make check`：case01~case08 全部 PASS；stderr 无交叉校验错误。
- `make methods`：两解法对照 4 项通过；`make extend`：扩展自测 12 项通过。
- 性能数据取 `-O2 -flto`、每项 3 次取平均的单次运行实测值。