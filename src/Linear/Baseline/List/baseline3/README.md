# 线性表（链表）Baseline 3 提交说明

本目录（`baseline3/`）完成 [课程 README](../README.md) 中 Baseline 任务 3「幽灵交叉路口」综合题。课程 README 为原始任务文档，不做改动；本文件包含目录结构、构建方式、源码组织，以及课程要求的两项材料：**进阶算法设计思路报告（含时空复杂度分析）** 与 **测试用例构造方式及程序输出**。

## 目录结构

```text
List/
├── README.md                 ← 课程原始任务文档（未改动）
└── baseline3/                ← 本提交目录（Baseline 任务 3）
    ├── README.md             ← 本提交说明
    ├── Makefile              ← make 生成 test/manual，make check 运行样例比对
    ├── src/
    │   ├── common/           ← 复用的依赖代码（非任务）
    │   │   └── basic_list/   ← 节点层：复用 baseline1 小任务 1 的裸节点链表（副本）
    │   │       ├── README.md
    │   │       ├── list.hpp / list.cpp
    │   ├── task1_algorithms/ ← 三个进阶算法 + RAII 场景持有者
    │   │   ├── README.md
    │   │   ├── ghost_intersection.hpp / ghost_intersection.cpp
    │   ├── task2_manual_test/ ← 手动构造 + 可视化验证（满足任务内容第 2 条）
    │   │   ├── README.md
    │   │   └── main_manual.cpp
    │   └── task3_command_test/ ← 命令式测试程序（编译为 test，满足提交表格）
    │       ├── README.md
    │       └── main_case.cpp
    ├── inputs/               ← 自测样例（case01 ~ case08）
    ├── outputs/              ← 与 inputs/ 同名的标准输出
    └── test / manual         ← make 生成的可执行程序
```

## 小任务划分

| 小任务 | 目录 | 可执行程序 | 内容 |
| --- | --- | --- | --- |
| 1 | `src/task1_algorithms/` | —（被下面两者链接） | 三个进阶算法 `detectAndRemoveCycle` / `getIntersectionNode` / `reverseSharedPath` + RAII 场景持有者 `Scenario` |
| 2 | `src/task2_manual_test/` | `manual` | 手动构造三类边界链表 + 命令行可视化 + 分步断言 |
| 3 | `src/task3_command_test/` | `test` | 命令式测试：读 `inputs/` 样例、求解、与 `outputs/` 逐用例比对 |

## 代码风格约定（与学习材料、baseline1/2 保持一致）

- 结点与工具函数沿用教程/`dsaac` 命名空间；三个算法按 README 指定的 camelCase（`detectAndRemoveCycle` 等）封装为 `GhostIntersection` 静态方法。
- 头文件只放声明，实现集中在 `.cpp`，用 `/* START: xxx implementation */` 与 `/* END */` 标注。
- 注释统一为中文，解释“为什么”（尤其记录了两处易错点的成因）。
- 迭代实现，不使用递归（避免 O(n) 栈空间）。

## 构建与自测

在 `baseline3/` 目录下执行：

```sh
make            # 一条命令编译生成 test 与 manual
make check      # 运行样例比对：case01.txt: PASS / ...
make unit       # 运行手动构造可视化测试
make asan       # ASan/LSan 内存检查：零泄漏、无双重释放
make clean      # 清理构建产物
```

说明：可执行文件名为 `test`（满足提交规范），因此运行样例的目标名为 `check`（避免与二进制同名冲突）。`test` 自动遍历 `inputs/` 与 `outputs/`，也支持单用例调试 `./test inputs/case01.txt`。

## 测试样例说明

| 样例 | 覆盖场景 |
| --- | --- |
| case01 | 课程 README 官方样例原样（CYCLE + NO_INTERSECTION + INTERSECTION） |
| case02 | CYCLE 环入口即头结点（entry=0，整表成环） |
| case03 | CYCLE 单结点自环 + 尾结点自环 |
| case04 | CYCLE entry=-1（无环）+ 空链表（`LIST EMPTY`） |
| case05 | NO_INTERSECTION 且两链表**值完全相同**（验证按指针而非按值判断） |
| case06 | INTERSECTION prefixA 为空（nA=0，头结点需更新） |
| case07 | INTERSECTION 两前缀皆空（headA == headB == boundary） |
| case08 | INTERSECTION 共享仅 1 结点（k=1，反转为恒等，须无自环） |

## 验证记录

- `make`：`test` 与 `manual` 编译通过，`-std=c++17 -Wall` 无警告。
- `make check`：case01 ~ case08 共 8 个用例全部 PASS，其中 case01 与课程 README 官方样例逐行一致。
- `make unit`：手动构造测试 6 项断言全部 `[PASS]`。
- `make asan`：`test` 与 `manual` 在 `-fsanitize=address,leak` 下均干净退出、无泄漏报告——重点验证了 case07（两前缀皆空、共享尾部）与 case03（自环）这两类最易双重释放/死循环的场景。

---

## 进阶算法设计思路报告

### 挑战 1：`detectAndRemoveCycle` —— 探查环路并解除

**思路**：分三阶段，均为 O(n) 时间 / O(1) 空间。

1. **判环**（Floyd 快慢指针）：慢指针每次一步、快指针每次两步。若无环，快指针先到链尾（`fast == nullptr` 或 `fast->next == nullptr`），返回 `nullptr`；若有环，快指针绕圈后必与慢指针相遇。
2. **定位入口**：相遇后把慢指针拨回链表头，两指针同速前进，再次相遇处即环入口。该结论基于「头结点到入口的距离」与「相遇点到入口沿环的距离」同余，无需额外记录结点即可推出。
3. **解环**：从入口沿环前进，找到 `next` 指回入口的结点（即环上最后一个结点）并将其 `next` 置空。**从入口起步而非从头起步**，使步数受环长约束，保证有限步终止；单结点自环、尾结点自环时循环体一次不执行，同样正确。

**为何不用哈希表**：用哈希表记录访问过的结点可在首次重复时找到入口，但需要 O(n) 额外空间，违反题目「禁用哈希表或额外数组」的 O(1) 空间规定。

### 挑战 2：`getIntersectionNode` —— 寻找交汇点

**思路（双指针，O(lenA+lenB) / O(1)）**：指针 `pA` 从 ListA 出发，走完 ListA 后切换到 ListB 头继续；`pB` 从 ListB 出发，走完 ListB 后切换到 ListA 头继续。两条指针走过的总长度均为 `lenA + lenB`：

- 若相交，`pA` 与 `pB` 会在交汇点相遇（因为相遇前各自走过的「独有前缀 + 公共段」恰好对齐）；
- 若不相交，两者会在同一轮同时变为 `nullptr`，此时 `pA == pB == nullptr`，循环结束返回 `nullptr`。

**关键细节**：判断依据是**指针身份**（`pA == pB`）而非结点值——两条链表可能含有相同值却不相交（见 case05，值同为 `1 2 3` 但结点对象不同）。切换指针必须写成 `pA = pA ? pA->next : headB;`，使比较能看到 `nullptr`；若写成 `pA = pA->next; if (!pA) pA = headB;`，比较永远看不到 `nullptr`，两链表不相交时死循环。

### 挑战 3：`reverseSharedPath` —— 空间逆转公共部分

**思路（O(n) / O(1)）**：因公共部分被两条链表物理共享，只需将这**一段**结点反转一次，两条链表看到的公共部分自然同时改变。

1. **先捕获**两条前缀的最后一个结点 `tailA`、`tailB`（前缀为空则为 `nullptr`）；
2. 用三指针法就地反转从交汇点开始的公共段，得到新头 `newHead`（原公共尾结点）；
3. 将 `tailA->next`（或前缀为空时更新 `headA`）指向 `newHead`，`tailB` 同理。

**两个必须处理的陷阱**：

- **顺序陷阱**：若「先反转、再沿前缀找尾结点」，反转已把交汇点的 `next` 置空并改变了共享段方向，前缀遍历会越过原前缀尾、直接走进已反转的共享段，重新接线时闭合成环，导致 `to_vector`/`free_memory` 永久挂死。故必须**反转前**捕获前缀尾。
- **头结点变化**：前缀为空时，该链表的头结点就是交汇点，反转后头结点应变为 `newHead`，因此 head 需按**引用**（`ListNode *&`）传入。

### 复杂度汇总

| 算法 | 时间复杂度 | 空间复杂度 |
| --- | --- | --- |
| `detectAndRemoveCycle` | Floyd 相遇 O(n) + 定位入口 O(n) + 解环 O(环长) ≤ **O(n)** | **O(1)** |
| `getIntersectionNode` | 两指针各走 ≤ lenA+lenB，即 **O(n)** | **O(1)** |
| `reverseSharedPath` | 两次前缀尾定位 O(前缀) + 反转 O(公共段长) ≤ **O(n)** | **O(1)** |

整体 O(n) 时间、O(1) 额外空间。**说明**：算法本身不借助哈希表或额外数组记录结点；`to_vector` 仅为**打印输出**而分配缓冲，不参与任何算法决策（含环链表打印时改用限步遍历，避免无限打印），不违反 O(1) 空间约束。

### 防泄漏的内存管理规则

题目要求的两类释放顺序错误通过 `Scenario` 的析构函数统一规避：

- **先解环再释放**：对仍成环的链表直接 `free_memory` 会沿环无限遍历；
- **共享尾部只释放一次**：相交链表两个头都 `free_memory` 会双重释放公共尾部。故各自前缀释放到 `boundary` 为止，公共部分只 `free_memory(boundary)` 一次；`boundary` 在反转后同步更新为公共段新头。

---

## 测试用例构造方式与程序输出

`task2_manual_test/main_manual.cpp` 在代码中手动构造三类链表并分步输出（`make unit` 的输出，实测记录）：

```text
===== 幽灵交叉路口：手动构造测试 =====

== 场景 1：存在环的单条链表 ==
  构造后的链表（含环）: 1 -> 2 -> 3 -> 4 -> 5 -> 3 -> 4 -> 5 -> (环) ...
  环入口值: 3
  解环后: 1 -> 2 -> 3 -> 4 -> 5 -> null
[PASS] 环入口值为 3
[PASS] 解环后链表为 1 2 3 4 5

== 场景 2：不相交的两条无环链表 ==
  ListA: 1 -> 2 -> 3 -> null
  ListB: 4 -> 5 -> null
  交汇点: NONE
[PASS] 不相交，交汇点为 NONE

== 场景 3：相交的两条无环链表 ==
  ListA（含共享尾部）: 10 -> 20 -> 7 -> 8 -> 9 -> null
  ListB（含共享尾部）: 30 -> 7 -> 8 -> 9 -> null
  交汇点值: 7
  反转公共部分后的 ListA: 10 -> 20 -> 9 -> 8 -> 7 -> null
  反转公共部分后的 ListB: 30 -> 9 -> 8 -> 7 -> null
[PASS] ListA 公共部分反转正确
[PASS] ListB 公共部分反转正确

合计：通过 6 项，失败 0 项
```

另有 8 个文件驱动样例（`inputs/` 对应 `outputs/`）覆盖更多边界，`make check` 全 PASS。其中 case01 是官方样例：

```text
CASE 1
CYCLE_ENTRY 3
LIST 1 2 3 4 5
CASE 2
INTERSECTION NONE
CASE 3
INTERSECTION 7
LIST_A 10 20 9 8 7
LIST_B 30 9 8 7
```