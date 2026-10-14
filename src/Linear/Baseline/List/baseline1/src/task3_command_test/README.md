# 小任务 3：命令式测试程序（按统一输入输出格式）

`test_command.cpp` 为测试程序：复用小任务 2 的模板链表类 `LinkedList<T>`，按 README 规定的命令式协议执行操作序列并比对标准输出。

## 需要完成的内容

- 按 Baseline 任务 1 的输入输出格式读取样例：初始长度、初始元素、操作次数与操作序列。
- 支持全部命令：`PUSH_FRONT`、`PUSH_BACK`、`INSERT`、`ERASE`、`ACCESS`、`FIND`、`CONTAINS`、`SIZE`、`PRINT`、`CLEAR`。
- 自动遍历 `inputs/` 中全部 `.txt` 样例，与 `outputs/` 中同名文件逐字符比对，输出 `caseXX.txt: PASS / FAIL`。
- 支持单用例模式：`./test inputs/case01.txt` 直接打印程序输出，便于调试。

## 行为要求

- 越界的 `ACCESS` / `INSERT` / `ERASE` 输出 `ERROR`，不中断后续操作。
- 布尔值输出 `true` / `false`；`PRINT` 空链表输出 `EMPTY`。
- 下标从 `0` 开始；`INSERT i x` 合法范围为 `[0, size]`。

## 验证方式

在提交目录（含 `inputs/`、`outputs/`）执行：

```sh
make
./test
```

## 样例覆盖说明

`inputs/` 中的样例覆盖：

- README 中的官方样例（`case01.txt`）；
- 空链表初始化与 `CLEAR` 后再打印（`EMPTY` 分支）；
- 头插、尾插、中间插入与删除混合操作；
- 越界访问（`ACCESS` / `INSERT` / `ERASE` 输出 `ERROR`）；
- `FIND` 查找头、尾、不存在元素；
- 反复删除至空表后再操作（验证尾指针与空表边界）。
