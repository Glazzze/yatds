# 阶段 4 · 测试与构建（T4.1–T4.4，T4.6）

日期：2026-10-28。交付 16 个样例（输入/标准输出同名配对）、`test` 比对
程序与完整 Makefile 目标；全部样例通过 ASan/UBSan 跑查。

## 产出

| 项 | 内容 |
| --- | --- |
| [inputs/](../inputs/) | `case01.txt`–`case16.txt`，覆盖普通插入、三连、多级连锁、不足三连、位置边界、撤销后再消除、五种道具、胜/负终局与错误输入（矩阵见 [samples.md](samples.md)） |
| [outputs/](../outputs/) | 与 `inputs/` 同名的 16 份标准输出；`make samples` 由主程序纯协议模式生成后**逐例人工核对** |
| [tools/test_runner.cpp](../tools/test_runner.cpp) | `test` 程序：遍历 `inputs/`，fork+exec 运行 `zuma --plain`（并设 `NO_COLOR=1`），与 `outputs/` 逐字节比对 |
| [Makefile](../Makefile) | `all` 一键编译 `zuma` + `test`；`check` 跑样例；`samples` 重建标准输出；`clean` 清理 |

## 样例集（T4.1/T4.2）

16 个用例的覆盖矩阵、关键期望与设计约定见 [samples.md](samples.md)。要点：

- 必做功能每项至少一例；五种道具（炸弹/变色/SWAP/HINT/REVERSE）各一例；
  胜/负三条路径（轨道清空、队列耗尽、`LIMIT` 到顶）齐备。
- 所有样例无随机性，输入头部/指令中可带 `#` 注释说明意图。
- 标准输出首版由 `make samples` 生成，随后对照规则逐例人工核对
  （消除轮次、`left/count/score`、撤销回填、终局与步数）。

## test 程序（T4.3）

- **定位**：按 `/proc/self/exe` 推算项目根，与当前工作目录无关；主程序
  与样例目录均相对项目根定位。
- **运行方式**：`fork` + `exec` 直接启动 `./zuma --plain`，stdin 重定向到
  输入文件、stdout 重定向到临时文件，**不经 shell**；子进程显式设置
  `NO_COLOR=1`，保证输出绝不含 ANSI 颜色码。
- **比对与报告**：逐字节比较；失败时打印首个差异行号及期望/实际内容
  （最多 5 处），并区分"缺标准输出/主程序异常/内容不一致"。
- **退出码**：`0` 全部通过；`1` 存在失败；`2` 环境缺失（如未编译）。

## Makefile（T4.4）

```sh
make            # 编译 zuma + test
make check      # 遍历 inputs/ 与 outputs/ 逐字节比对
make samples    # 维护用：重新生成 outputs/（之后须人工核对）
make selftest   # core 20 场景
make drivertest # driver 21 用例 + 颜色开关断言
make clean      # 清理全部构建产物
```

## 验证链（全部通过）

```sh
make                         # -Wall -Wextra 零警告
make check                   # 16/16 PASS
make selftest                # 20 场景 PASS
make drivertest              # 21 用例 PASS
```

T4.6：以 `-fsanitize=address,undefined` 重新编译主程序，逐一跑 16 个
样例——全部无 sanitizer 报告，且输出与 `outputs/` 逐字节一致。

T4.5（可选项，未实现）：独立 Python oracle 未落地；当前以"标准输出
逐例人工核对 + 阶段 2 的 10⁵ 随机差分/20 场景自测"承担自洽校验风险。
若后续需要，可在阶段 5 双实现交叉验证（T5.6）中一并补齐。

## 顺带勘误

PROTOCOL.md §3.2 原写 `SHOOT <pos> <ball>` 回显"实际发射的球符号"，
但 driver 一直按输入原样回显（普通球无颜色参数）。已按实现勘误并记录
在协议变更说明中；样例 `outputs/` 以实际行为为准。

## 阶段 4 收口

- [x] T4.1 样例集 16 个（矩阵覆盖全部要求项）
- [x] T4.2 同名标准输出生成并人工核对
- [x] T4.3 `test` 程序（PASS/FAIL 逐例 + 汇总 + 差异定位）
- [x] T4.4 Makefile：`make` / `make check` / `make clean`（另有 `samples`）
- [x] T4.6 ASan/UBSan 全量样例通过
- [ ] T4.5（可选）独立 oracle —— 暂缓，理由见上