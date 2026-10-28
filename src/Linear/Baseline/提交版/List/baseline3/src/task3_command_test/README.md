# 命令式测试程序（编译为 test）

`main_case.cpp` 编译为 `test`，按课程 README 规定的输入输出格式读取 `inputs/` 中的样例，对每个场景求解「幽灵交叉路口」综合题，并与 `outputs/` 的标准输出逐用例比对。

## 输入格式

- 第 1 行 `T`（场景数量），随后 `T` 个场景，每个场景第 1 行为场景类型：

| 场景 | 后续输入 |
| --- | --- |
| `CYCLE` | `n entry`；下一行 `n` 个结点值（`entry` 为环入口下标，`-1` 表示无环） |
| `NO_INTERSECTION` | `nA` 及 `nA` 个值；再 `nB` 及 `nB` 个值 |
| `INTERSECTION` | `nA` 及 ListA 独有前缀；`nB` 及 ListB 独有前缀；`k` 及共享公共尾部 |

## 输出格式

| 场景 | 输出 |
| --- | --- |
| 所有场景 | 先输出 `CASE t`（`t` 从 1 开始） |
| `CYCLE` | `CYCLE_ENTRY <入口值｜NONE>`，随后输出解环后的链表 `LIST ...` |
| `NO_INTERSECTION` | `INTERSECTION NONE`（不输出链表） |
| `INTERSECTION` | `INTERSECTION <交汇值>`，随后 `LIST_A ...` 与 `LIST_B ...`（公共部分反转后） |
| 链表打印 | 元素空格分隔，空链表输出 `EMPTY` |

## 实现要点

- 解析统一用 `operator>>`（自动跳过空白与换行）：`n = 0` 时无论第 2 行空行是否存在都能正确对齐，避免按行读取时的空行错位。
- **输入健壮性**：`entry` 越界（含 `n == 0` 而 `entry != -1`）时按无环处理，避免 `access` 返回 `nullptr` 后被解引用。
- 链表始终由 `Scenario` 持有，析构自动解环并正确释放（共享尾部只释放一次）。
- 复用 baseline1 `task3_command_test` 的目录遍历（POSIX `opendir`/`readdir` + 排序 + 整串比对）。
- 单用例调试：`./test inputs/case01.txt` 直接打印该样例的完整输出。

## 验证

```sh
make check     # 自动遍历 inputs/ 与 outputs/ 比对：case01~case08 全 PASS
```