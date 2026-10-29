# 网页版测试记录（阶段 5b）

> 历史验收记录（2026-10-29 早期版本）；本轮规则与测试更新见 [优化验证.md](优化验证.md)。
> 日期：2026-10-29。执行环境：Linux 容器（无本地显示）、Node v22.22.1、
> Playwright 1.64.1、Chromium 156.0.8078.4（headless）。本文区分**已自动
> 执行的验证**与**仍需人工复核的检查**，不把 DOM shim 或截图脚本的结果
> 直接写成“人工视觉验收已完成”。

## 1. 自动化验证

在 `src/Linear/Project/` 下执行：

```bash
make check        # C++ inputs/outputs 逐字节比对
make selftest     # core 算法自测（25 场景）
make difftest     # core 随机差分（默认 10⁵ 局，固定种子）
make drivertest   # driver 协议集成测试
make webverify    # web-samples + webcheck + webtest + webstatic + webdom
make browsercheck # 真实 Chromium 验收（可选依赖，见 §2）
```

| 检查 | 结果 | 覆盖 |
| --- | --- | --- |
| `make check` | 16 / 16 PASS | 全量样例的 CLI 逐字节输出 |
| `make selftest` | 25 场景 PASS | 装载、三连、连锁、炸弹、变色、交换、倒退（含右拼缝）、提示、LIMIT、撤销、34/60 轮深连锁、终局守卫等 |
| `make difftest` | 100000 / 100000 PASS | 随机 SHOOT/SWAP/REVERSE：U2、非法零副作用、分数增量、撤销还原、重放确定性 |
| `make drivertest` | 21 用例 PASS | 协议回显、错误短语、终局、HELP、大小写、NO_COLOR/UI |
| `make webcheck` | 16 / 16 PASS | JS 协议引擎 vs `./zuma --plain`，逐事件 token 比对 |
| `make webtest` | 22 项 PASS | JS 引擎/协议/Controller 单测；400 局随机操作全量撤销还原；深连锁饱和与右拼缝回归 |
| `make webstatic` | 16 项 PASS | id 唯一/引用、脚本顺序、本地资源、SVG 图标、JS 语法、CSS/HTML 结构 |
| `make webdom` | 10 组主流程 PASS | app/view 集成冒烟（见 §1.2） |
| `make browsercheck` | 62 / 62 PASS | 真实 Chromium：布局、主流程、弹窗、键盘、reduced-motion、灰度可读性、file:// 与 HTTP（见 §2） |

### 1.1 `webcheck` 的双实现范围

交叉验证逐项比对：header 回显、READY、每次 SHOOT/REVERSE 的
`round/left/count/points/balls/trackAfter`、TRACK、QUEUE、ERROR 原因、
HINT、HELP、终局 `status/score/moves`。网页侧的暂停、历史、随机、
音效不参与协议比对。

### 1.2 `webdom` 的冒烟流程

`tools/web_dom_smoke.js` 内置极简 DOM shim（元素、选择器、事件冒泡/
捕获、尺寸桩），按 `index.html` 的脚本顺序执行，覆盖：

1. 初始装载 case03、轨道/队列/计分/标尺静态渲染，默认中点与 `1`/`0` 首尾选位；
2. 空格发射 → 两轮连锁动画 → 胜利结算（110 分 / ×2 / 7 球）；
3. 结算页撤销 → 轨道、队列、计分、历史、按钮状态恢复；
4. 自选局面 + 确认弹窗 + 变色球选色（数字键）→ 消除；
5. 历史抽屉 → 只读预览 → 返回当前；
6. 关卡切换 → REVERSE 区间点选 → 确认反转 → 消除；
7. 暂停/继续、音效开关与 localStorage、帮助弹窗；
8. SWAP → HINT → UNDO 按钮链路；
9. LIMIT 失败结算 → 重新开始；
10. 随机生成器预览（固定种子）→ 载入并核对轨道/队列。

该 shim **不执行 CSS、不做布局测量**，因此只证明脚本装配与状态流转，
不替代 §2 的真实浏览器检查。

### 1.3 静态检查

`tools/web_static_check.js`（`make webstatic`）自动校验：

- 网页必需文件齐全；HTML 的 110 个 `id` 无重复。
- `view.js` 的 `IDS` 列表与 22 个 `getElementById` 字面量均存在于 HTML。
- 7 个脚本按 `engine → protocol → samples-data → audio → controller →
  view → app` 顺序加载，文件均存在；CSS/图标引用均存在。
- 页面无外部 CDN/字体/图片，无 `type="module"`、无 `fetch(` 依赖。
- 14 个 SVG 图标引用均有 `<symbol>` 定义。
- 7 个网页脚本通过语法解析；`app.css` 大括号平衡；HTML 标签配对。
- 内嵌样例恰为 16 个。

## 2. 真实浏览器验收

`tools/web_browser_acceptance.js` 用 Playwright 启动**真实 Chromium**
（不是 DOM shim），执行 CSS 布局、动画、焦点、媒体查询并截图。

### 2.1 安装与运行

```bash
cd src/Linear/Project
npm install --no-save playwright        # 只装到 node_modules/，不改 package.json
npx playwright install chromium         # 下载浏览器
npx playwright install-deps chromium    # 容器/首次运行所需系统库
make browsercheck                       # 等价于 node tools/web_browser_acceptance.js
```

任意一项失败时脚本以退出码 1 结束，并在终端逐条列出 FAIL；缺少
Playwright 时以退出码 2 结束并打印上面的安装命令。`browsercheck`
**不并入 `webverify`**，因为浏览器与系统库属于可选依赖，核心验证链
不应因未安装 Chromium 而失败。

输出：

- `docs/browser-acceptance/*.png`：17 张截图；
- `docs/browser-acceptance/report.json`：每项检查的 `ok/label/detail`、
  浏览器版本、Tab 顺序与实际截图清单。

### 2.2 本轮结果

2026-10-29 在 Chromium 156.0.8078.4 上 **62 / 62 项通过**，无控制台
错误、无页面异常、无资源加载失败。覆盖：

1. **两种打开方式**：`file://` 直开与本地 HTTP（脚本内置静态服务器），
   并断言 HTTP 页面的全部资源来自本地。
2. **四档宽度** 1232 / 1020 / 760 / 560 px：无页面横向溢出；顶栏与棋盘
   不重叠；棋盘位于视口内；轨道有球且每颗球都有球面字母；控制条与
   队列区域可见。
3. **主流程**：case03 点击空隙发射 → 两轮连锁 → 胜利结算
   （110 分 / 1 步 / ×2 / 7 球）→ 结算页撤销 → 恢复 `RRBBRR`、队首 `B`、
   0 分 0 步，6 颗球重新可见且彩带已清除。
4. **弹窗与道具**：帮助弹窗 Tab 焦点陷阱与 Esc 关闭；关卡弹窗；
   REVERSE 区间点选与消除；变色球数字键定色；队列耗尽失败；
   SWAP、HINT、炸弹、LIMIT 失败。
5. **键盘**：22 次 Tab 的顺序覆盖关卡、历史、音效、暂停、帮助、
   撤销、提示、交换、倒退。
6. **`prefers-reduced-motion: reduce`**：新上下文生效，胜利结算在 1.2 s
   内完成（不等待长动画）。
7. **色盲可读性**：将轨道灰度化后截图，仍能读到每颗球的字母
   （颜色 + 字母双编码）。
8. **运行环境清洁**：file://、HTTP、reduced-motion 三个页面均无
   console error / pageerror / requestfailed。

### 2.3 本轮发现并修复的浏览器专属缺陷

这些问题在 DOM shim 与静态检查中都不会暴露，只有真实 CSS/布局/焦点
才会出现：

| 现象 | 根因 | 修复 |
| --- | --- | --- |
| `hidden` 的预览条、区间条、提示徽标仍显示且可聚焦 | 组件 `display` 规则覆盖了 HTML `hidden` | `app.css` 增加 `[hidden] { display: none !important; }` |
| 历史抽屉关闭时，`history-close` 仍进入 Tab 顺序 | 抽屉仅移出视口，元素仍可聚焦 | 抽屉增加 `visibility: hidden`，`.is-open` 时恢复 |
| REVERSE 模式下点不到轨道球（元素被拦截） | `.track-slots` 容器铺满轨道并接收指针 | 容器 `pointer-events: none`，`.slot` 恢复 `auto` |
| 发射器 `aria-hidden="true"` 却是可聚焦按钮 | HTML 与脚本行为矛盾 | 改为 `role="button" tabindex="0" aria-label="发射器…"` |
| 结算页撤销后彩带仍盖在帮助/关卡弹窗上方 | 彩带 `z-index` 高于弹窗，且只在 2.3 s 后自动移除 | `closeModal("result-modal")` 调用 `clearCelebration()` |

### 2.4 仍需人工复核的项目

headless Chromium 能验证渲染、布局与焦点，但以下项目仍建议由人在
桌面浏览器中过一遍：

- [ ] Firefox / Safari / Edge 的实际渲染与 `file://` 行为（本轮只跑 Chromium）；
- [ ] 真实屏幕阅读器（NVDA / VoiceOver）朗读轨道、队列、结算与 live region；
- [ ] 纯键盘完整游玩一局（不只看 Tab 顺序，还看选中位与发射是否顺手）；
- [ ] 浏览器 80% / 100% / 125% / 150% 缩放下的标尺、幽灵球与瞄准线；
- [ ] 红绿色弱模拟下颜色 + 字母的实际辨识度（自动化只验证了灰度字母仍在）；
- [ ] 音效实际输出与首次用户手势创建 AudioContext；
- [ ] `file://` 下“复制输入”的剪贴板回退，以及触屏点击/拖动体感。

人工复核方式：在 `src/Linear/Project/` 执行
`python3 -m http.server 8000 --bind 127.0.0.1`，打开
`http://127.0.0.1:8000/web/index.html`；另开一份
`file:///.../web/index.html`。DevTools 的 Rendering 面板可模拟
`prefers-reduced-motion` 与“红绿色弱/灰度”视觉缺陷。

> **远程环境注意**：如果开发环境是 code-server 容器，本机的
> `file:///home/...` 与 `localhost:8000` 指向的是你本机而不是容器。
> 此时用平台端口代理（code-server 通常为
> `https://<host>/proxy/8000/web/`，VSCode Remote 用 Ports 面板转发）。
> 自动验收脚本在容器内直接访问 `file://` 与临时 HTTP 端口，不受此限制。

## 3. 已知限制

- 真实浏览器验收目前只覆盖 Chromium；Firefox/Safari/Edge 与真实屏幕
  阅读器仍属人工复核项。
- `webdom` 只覆盖逻辑/状态层，不能替代 §2 的渲染与焦点检查。
- Web Audio 在部分浏览器首次播放需用户手势；已由“默认关闭 + 手动开启”
  规避自动播放限制。
- `file://` 下剪贴板 API 可能受限；随机生成器的“复制输入”带
  `document.execCommand` 回退，失败时提示手动选择。
- 浏览器截图依赖字体与平台渲染，跨机器可能有个位数像素差异；断言使用
  布局阈值（±2 px）而非像素级截图比对。