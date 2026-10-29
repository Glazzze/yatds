# 独立提交版说明

更新日期：2026-10-29。

## 交付内容

Project 包含独立构建所需全部源代码。`src/core/linked_stack.hpp` 自实现
`zuma::LinkedStack<T>`，`Game` 直接使用 `LinkedStack<UndoEntry>`；已移除
外部链栈的 include/编译路径及 `src/linked_stack_undo.cpp`。
模板定义完整置于头文件，无需另行链接或手工显式实例化。

栈以单向节点保存撤销记录，提供 `push/top/pop/empty`；禁止复制以避免
重复释放，析构迭代释放所有节点。链操作 O(1)，复制变长撤销记录的成本另计。
另修复两个测试程序的可执行文件定位：macOS 使用 `_NSGetExecutablePath`，
Linux 使用 `/proc/self/exe`，可以从 Project 之外的工作目录运行测试。
Makefile 跟踪头文件依赖，修改本地容器头文件后会重新编译相关目标。

## 运行

需要 C++17 编译器、Make 与 macOS/Linux 的 POSIX 接口。
网页自动化另需 Node ≥18；直接打开网页不需要 Node 或 Playwright。

```sh
cd /path/to/Project
make
make check stacktest selftest difftest drivertest
make webverify   # 安装 Node 后可选运行
```

源码包不包含 `node_modules/` 或平台相关可执行文件。`make` 生成名为 `test`
的样例比对程序与 `zuma`；在其他操作系统上应重新构建。
可选真实浏览器验收的依赖安装方式见 README。

## 历史材料

实验报告中的 Prompt 截图和阶段 0 记录保留早期复用方案，仅作为协作过程
证据。当前代码、Makefile、README、设计文档和报告最终实现章节已同步为
项目内链式栈；这些历史材料不会引入构建依赖。

## 验证

本轮本机已通过 25 场景 core 自测、100000 局固定种子随机操作测试、
21 组 driver 集成测试及 4 项颜色断言、16 个输入输出样例和
16 个 C++/JS 协议交叉验证用例。新增 `make stacktest` 覆盖空栈异常、
LIFO、只读栈顶、移动值、复制失败保持原栈顶与十万节点析构。

已在系统临时目录中仅复制 Project 源码（无父目录依赖、node_modules 或旧
二进制）重新构建，通过全部上述 C++ 检查及 `webverify`（协议交叉 16、
JS 22、静态 16、DOM 10 组）。两个测试程序从 `/tmp` 启动亦全部通过。
新增链栈专项和 25 场景 core 自测分别通过 ASan/UBSan，无诊断。
本轮评分与后续优化方向见 [评分检查.md](评分检查.md)。


后续五项优化与当前测试计数见 [优化验证.md](优化验证.md)；上述隔离构建
记录保留独立化完成时的历史结果。提交材料由用户手工整理，本轮不更新压缩包。
