# 祖玛游戏

本目录包含 PDF 实验报告、C++ 源码、16 组输入输出样例、测试程序源码、
静态网页及设计和验收文档，可单独复制后运行，不依赖父目录源码。

## 文件结构

```text
24325187_刘雅静/
├── 实验报告.pdf
├── README.md
├── Makefile
├── src/       # C++ 游戏引擎、链表/队列/链栈及 CLI
├── inputs/    # case01.txt–case16.txt
├── outputs/   # 同名标准输出
├── tools/     # 测试与规模观测源码、脚本
├── web/       # index.html 与 assets/ 全部资源
└── docs/      # 协议、设计与验收证据
```

## 运行环境

- C++：C++17 编译器、Make；测试工具使用 macOS/Linux 的 POSIX 接口。
- 网页：现代浏览器；直接打开不需要 Node、服务器或安装依赖。
- 网页自动化：Node ≥18；真实浏览器验收另需可选 Playwright/Chromium。

本提交版不附 `zuma`、`test` 或 `tools/` 下的编译产物。进入包含
`Makefile` 的提交版根目录后，执行 `make`，即可在当前系统上生成主程序
`zuma` 与样例测试程序 `test`。

测试程序源码为 `tools/test_runner.cpp`，配套头文件为
`tools/executable_path.hpp`；撤销栈完整实现于 `src/core/linked_stack.hpp`。
网页直接打开不受 C++ 是否编译影响。

## 编译与 C++ 测试

在包含 `Makefile` 的提交版根目录执行：

```sh
make                  # 从源码生成 zuma 与 test
make check            # 16组 inputs/outputs 逐字节比对
```

也可直接执行 `make check`，Makefile 会先生成缺失的 `zuma`、`test`，
再运行样例比对。需要强制重新编译全部源文件时使用 `make -B`。

```sh
./zuma --plain < inputs/case03.txt
./zuma --ui < inputs/case03.txt
make stacktest        # 自实现链式栈专项
make selftest         # core 27场景
make difftest         # 独立字符串参考差分，默认100000局
make drivertest       # driver 27用例与4项颜色断言
```

以上命令均从包含 Makefile 的提交版根目录执行。`make clean` 会清除
可执行文件，随后 `make` 可重新生成 `zuma` 与 `test`。

## 网页运行

网页有以下三种运行方式，均不需要先编译 C++ 或执行 `npm install`。
以下命令中的 `/解压后的父目录/24325187_刘雅静`、用户名和服务器地址
需要替换为自己的实际信息。项目根目录是包含 `Makefile`、`web/` 的目录。

### 方式一：本机直接用浏览器打开文件

适用于项目文件已经下载、解压到本机的情况，无需启动服务器。

1. 在文件管理器中进入 `24325187_刘雅静/web/`。
2. 右键 `index.html`，选择“打开方式”，使用 Chrome、Safari、Edge 等浏览器打开。
3. 浏览器地址栏应显示 `file:///…/24325187_刘雅静/web/index.html`。

macOS 也可以在终端输入：

```sh
cd "/解压后的父目录/24325187_刘雅静"
open web/index.html
```

如果 `open` 打开的是编辑器，改为右键选择浏览器，或将 `index.html`
拖入浏览器窗口。VS Code 的 Markdown 预览链接可能经过
`vscode-resource` 转换，直接运行请按以上步骤用浏览器打开实际文件。

### 方式二：本机启动 HTTP 服务器

适用于本机已安装 Python 3，希望通过 HTTP 地址访问网页的情况。

在本机终端输入：

```sh
cd "/解压后的父目录/24325187_刘雅静"
python3 -m http.server 8000 --bind 127.0.0.1
```

看到 `Serving HTTP on … port 8000` 后，保持该终端运行，打开浏览器，
在地址栏输入：

```text
http://127.0.0.1:8000/web/
```

也可访问 `http://localhost:8000/web/`。macOS 如需用命令打开，
在另一个终端窗口输入：

```sh
open "http://127.0.0.1:8000/web/"
```

结束游玩后，在运行服务器的终端按 `Ctrl+C` 停止服务。
如果提示 8000 端口被占用，将启动命令和访问地址中的端口一起改为 8001。

### 方式三：远程服务器运行，本机浏览器访问

适用于项目位于远程服务器、容器或 VS Code Remote 环境的情况。
远程路径中的 `index.html` 不能通过本机文件管理器直接打开，需转发服务端口。

**第一步：在远程终端启动服务器。** 路径替换为服务器上的项目路径：

```sh
cd "/服务器上的项目路径/24325187_刘雅静"
python3 -m http.server 8000 --bind 127.0.0.1
```

保持该远程终端运行。

**第二步：将远程 8000 端口转发到本机。** 根据连接方式选择一种：

- **通过 SSH 连接服务器**：在本机另开终端，输入以下命令，并完成登录：

  ```sh
  ssh -N -L 8000:127.0.0.1:8000 用户名@服务器地址
  ```

  命令保持运行、没有额外输出通常是正常现象。随后在本机浏览器访问
  `http://127.0.0.1:8000/web/`。如果本机 8000 已被占用，使用
  `ssh -N -L 8001:127.0.0.1:8000 用户名@服务器地址`，浏览器改为访问
  `http://127.0.0.1:8001/web/`，远程服务仍使用 8000。

- **通过 VS Code Remote 连接**：打开底部“端口 / Ports”面板，选择
  “转发端口 / Forward a Port”，输入 `8000`；打开面板提供的本地转发地址，
  并在地址末尾加 `/web/`。本地端口以面板实际显示的值为准，无需额外运行 SSH 转发命令。

- **使用 code-server 或在线开发平台**：打开平台提供的 8000 端口预览/代理地址，
  并进入 `/web/`。code-server 的常见形式为
  `https://你的code-server地址/proxy/8000/web/`，以平台实际地址为准。

这里的 `localhost` 或 `127.0.0.1` 指访问者所在的计算机；在本机浏览器中
访问远程服务前，需要先完成端口转发或使用平台代理地址。
结束后，在远程服务终端按 `Ctrl+C`；SSH 转发方式还需在本机转发终端按 `Ctrl+C`。

三种方式打开后均默认载入 case03，支持选关、自选/随机局面、点击或键盘选位、
发射、炸弹、变色球、交换、提示、区间反转、撤销、历史回看和结算。

## 网页与规模验证

```sh
make webverify        # 样例内嵌、跨语言样例/随机、JS、静态、DOM验证
make scalecheck       # 100/500/1000初始轨道球、2000队列球的规模观测
```

`make webverify` 包含：`web-samples`、`webcheck`、`webrandom`、`webtest`、
`webstatic`、`webdom`。Node 不在 PATH 时，可使用
`make webverify NODE=/实际路径/node` 指定解释器。
`webcheck`、`webrandom` 会先生成缺失的 `zuma`；`scalecheck` 会先编译
`tools/scale_probe`，再更新 `docs/scale-observation.json`。这些可执行文件
均为本地验证时产生的文件，不属于提交目录的源码清单。

真实浏览器验收为可选项：

```sh
npm install --no-save playwright
npx playwright install chromium
make browsercheck
```

Linux 如缺少浏览器系统库，可另执行 `npx playwright install-deps chromium`。
`browsercheck` 运行时会创建 `docs/browser-acceptance/` 并写入截图及报告；
该目录是现场验收产物，不是本提交包的必备文件。

## 报告与设计证据

- [实验报告.pdf](实验报告.pdf)：完整报告，已嵌入30张过程与运行截图。
- [docs/PROTOCOL.md](docs/PROTOCOL.md)：规则、输入输出与终局语义。
- [docs/design.md](docs/design.md)：数据结构选型、算法、撤销与成本分析。
- [docs/web-design.md](docs/web-design.md)：网页视觉和交互设计。
- [docs/web-test.md](docs/web-test.md)：网页验收方法、历史结果和覆盖边界。
- [docs/优化验证.md](docs/优化验证.md)：本轮优化、复现命令与测试结论。
- [docs/scale-observation.json](docs/scale-observation.json)：规模观测原始数据。
- [docs/optimization-verification.log](docs/optimization-verification.log)：测试输出。
- [docs/optimization-asan.log](docs/optimization-asan.log)：ASan/UBSan输出。
- [docs/optimization-browser-report.json](docs/optimization-browser-report.json)：浏览器逐项结果。

本包按最终提交结构精简，不附报告Markdown、单独截图目录和历史阶段文件。
PDF内现有文件链接已改为相对路径；未附的 stage0、stage3、TASKS 历史材料
链接改为报告内对应协作章节。PDF阅读器对打开Markdown文件的支持可能不同，
也可按上面的路径直接查看。
