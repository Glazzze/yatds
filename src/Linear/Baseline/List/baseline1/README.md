# 线性表（链表）Baseline 1 提交说明

本目录（`baseline1/`）完成 [课程 README](../README.md) 中 Baseline 任务 1（链表的基本实现）的三个小任务。课程 README 为原始任务文档，不做改动；本文件合并了目录结构、构建方式、源码组织、代码风格、测试与结果分析，作为提交材料的完整说明。

## 目录结构

```text
List/
├── README.md             ← 课程原始任务文档（未改动）
└── baseline1/            ← 本提交目录（Baseline 任务 1）
    ├── README.md         ← 本提交说明
    ├── Makefile          ← make 生成 test，make check 运行样例比对
    ├── src/
    │   ├── task1_basic_list/   ← 小任务 1：教程风格的裸节点链表实现
    │   │   ├── README.md
    │   │   ├── list.hpp
    │   │   ├── list.cpp
    │   │   └── test_list.cpp
    │   ├── task2_linked_list/  ← 小任务 2：LinkedList<T> 模板链表类（任意类型）
    │   │   ├── README.md
    │   │   ├── linked_list.hpp
    │   │   ├── linked_list.cpp
    │   │   └── test_linked_list.cpp
    │   └── task3_command_test/ ← 小任务 3：命令式测试程序（统一 I/O 格式）
    │       ├── README.md
    │       └── test_command.cpp
    ├── inputs/           ← 自测样例（case01 ~ case03）
    ├── outputs/          ← 与 inputs/ 同名的标准输出
    └── test              ← make 生成的比对程序
```

## 小任务划分

| 小任务 | 目录 | 内容 |
| --- | --- | --- |
| 1 | `src/task1_basic_list/` | 教程风格的 `ListNode` 裸节点实现：初始化（`build_list`）、`insert` / `remove` / `access` / `find` 四个教程基本操作，以及 `push_front` / `push_back` / `length` / `contains` / `reverse` / `to_vector` / `free_memory` 扩展函数，配单元式验证程序 `test_list.cpp` |
| 2 | `src/task2_linked_list/` | `LinkedList<T>` 模板类：RAII 内存管理（析构释放、深拷贝、自赋值安全）、尾指针优化 `push_back` 至 O(1)、越界抛 `std::out_of_range`，配 int / double / char 三种类型实例化的测试程序 `test_linked_list.cpp` |
| 3 | `src/task3_command_test/` | 复用 `LinkedList<T>` 的命令式测试程序：按课程 README 规定的统一输入输出格式读取 `inputs/` 样例、执行操作序列、与 `outputs/` 标准输出逐用例比对 |

## 三个小任务的层次关系

```text
task1_basic_list/（教程裸节点，教学演示用）
        │  小任务 2 将同一逻辑封装为模板类
        ▼
task2_linked_list/（LinkedList<T>，RAII + 尾指针）
        │  小任务 3 直接 #include "../task2_linked_list/linked_list.hpp"
        ▼
task3_command_test/（统一 I/O 格式的比对程序）
```

小任务 3 的 `test_command.cpp` 通过相对路径引用小任务 2 的头文件，因此三组代码保持“底层实现 → 高层封装 → 外壳程序”的层次关系，符合“完成一个模块后在其上构建”的工程习惯。

## 代码风格约定（与学习材料保持一致）

- 节点命名沿用教程：`ListNode`（`val` + `next` + 构造函数）。
- 命名空间 `dsaac` 包裹实现，再通过 `using` 暴露到全局，方便直接调用。
- 头文件只放声明（`.hpp`），实现集中在同目录 `.cpp`，用 `/* START: xxx implementation */` 与 `/* END */` 标注实现区间。
- 注释统一为中文，解释“为什么”而不只是“做了什么”。
- 测试程序使用 `check` / `expect` 风格断言函数，逐项打印 `[PASS]` / `[FAIL]`，便于人工核对。

## 构建与自测

在 `baseline1/` 目录下执行：

```sh
make           # 编译生成 test（命令式比对程序）
make check     # 运行样例比对：case01.txt: PASS / ...
make unit      # 编译并运行小任务 1、2 的单元验证程序
make clean     # 清理构建产物
```

说明：

- 可执行文件名为 `test`（满足提交规范“提交名为 test 的测试程序”）。因目标名 `test` 已被该二进制占用，**运行样例比对的目标名为 `check`**（避免同名伪目标与文件目标冲突导致 make 循环依赖）。
- `test` 程序自身会自动遍历 `inputs/` 与 `outputs/`，逐用例给出 `PASS` / `FAIL`；也支持单用例调试模式：`./test inputs/case01.txt` 直接打印程序输出。
- 单元验证程序输出到 `/tmp`，避免污染提交目录。

## 测试样例说明

| 样例 | 覆盖场景 |
| --- | --- |
| case01 | 课程 README 给出的官方样例原样 |
| case02 | 空链表初始化（n=0）：`SIZE` / `PRINT`（EMPTY 分支）、头插尾插后越界访问、`FIND` / `CONTAINS` 查找存在与不存在元素、`CLEAR` 后再操作 |
| case03 | 越界组合：`ACCESS` 越界与负下标、`INSERT` 越界（合法范围 [0, size]）、连续 `ERASE` 至空表、`FIND` 查找头/不存在元素、删空后 `SIZE` / `PRINT` |

## 验证记录

- `make`：编译通过，`-std=c++17 -Wall` 无警告。
- `make check`：case01 ~ case03 共 3 个用例全部 PASS。
- `make unit`：小任务 1、2 共 46 项断言全部 `[PASS]`。
- **ASan / LSan 内存检查**：三个程序（`test_list`、`test_linked_list`、`test`）在 `-fsanitize=address,leak` 下全部干净退出，无内存泄漏、无未定义行为。

复现内存检查的命令（在 `baseline1/` 目录执行）：

```sh
g++ -std=c++17 -Wall -fsanitize=address,leak src/task1_basic_list/list.cpp src/task1_basic_list/test_list.cpp -o /tmp/a1 && /tmp/a1
g++ -std=c++17 -Wall -fsanitize=address,leak src/task2_linked_list/linked_list.cpp src/task2_linked_list/test_linked_list.cpp -o /tmp/a2 && /tmp/a2
g++ -std=c++17 -Wall -fsanitize=address,leak src/task3_command_test/test_command.cpp src/task2_linked_list/linked_list.cpp -o /tmp/a3 && /tmp/a3
```

## 程序与结果分析

### 小任务 1：裸节点链表

- **实现要点**：节点定义、`insert` / `remove` / `access` / `find` 与课程教程逐行一致（`insert` 只改写两条引用、`remove` 负责 `delete` 被删节点）；扩展操作遵循同样风格——`build_list` 尾插法建表、`reverse` 三指针就地反转（O(n) 时间 / O(1) 额外空间）。
- **测试设计**（体现函数特性，而非简单功能测试）：`insert` 后用 `access` 交叉验证“被断言的正是刚插入的节点”；`access(-1)` 验证与教程实现一致的边界行为（直接返回头节点）；反转操作专测头、尾两个最易出错的边界指针；空表/单节点反转不崩溃。
- **内存管理**：`remove` 单点释放 + `free_memory` 整表释放，经 ASan/LSan 验证零泄漏。

### 小任务 2：模板链表类

- **实现要点**：`LinkedList<T>` 将“头节点代称整表”的裸节点版本封装为 RAII 类——构造/拷贝/析构统一管理节点内存，调用方无须手动释放；`Node` 嵌套于类内，外界无法绕过类直接操作节点。
- **尾指针优化**：额外维护 `tail_` 使 `push_back` 从 O(n) 降为 O(1)——这是对教程版裸节点 `push_back` 需遍历全表的主要改进。删除头/尾节点、`reverse`、`clear` 时均同步维护 `tail_`，测试第 7 节“混合操作压力”专门验证该一致性。
- **异常语义**：越界 `at` / `insert` / `erase` 抛 `std::out_of_range`，与 `std::vector::at` 行为对齐；测试第 6 节逐项捕获验证。
- **深拷贝**：拷贝构造/赋值逐节点复制，测试第 5 节验证“修改副本不影响原表”与自赋值安全——裸节点版本浅拷贝会共享节点导致 double free，这正是封装为类的必要性所在。
- **模板验证**：int / double / char 三类型实例化（`.cpp` 中显式实例化保证分编译单元链接通过），证明“任意数据类型的存储”。

### 小任务 3：命令式测试程序

- **实现要点**：复用小任务 2 的 `LinkedList<T>`，驱动层按课程 README 规定的协议解析 10 种命令并收集输出；比对层自动遍历 `inputs/` 与 `outputs/` 同名文件逐字符比对。
- **边界语义**：越界的 `ACCESS` / `INSERT` / `ERASE` 输出 `ERROR` 但**不中断后续操作**（区别于小任务 2 的抛异常——输出规范要求打印 ERROR 而非终止）；`PRINT` 空表输出 `EMPTY`；`INSERT i x` 合法范围为 `[0, size]`（i == size 等价尾插）。
- **健壮性**：下标按 `long long` 读取再校验范围，负数下标统一走 `ERROR` 分支；未知命令静默忽略。

### 结果

`make check` 3 个用例全部 PASS，其中 case01 与课程 README 官方样例逐行一致；两套单元验证共 46 项断言全通过。三种验证途径（样例比对、单元断言、ASan）互相独立，共同证明实现的正确性与内存安全性。