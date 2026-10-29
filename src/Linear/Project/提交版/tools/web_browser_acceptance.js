/*
 * 祖玛 · 真实浏览器验收（Playwright + Chromium）
 *
 * 与 web_dom_smoke.js 的区别：本脚本启动真实 Chromium，执行 CSS 布局、
 * 动画、焦点、媒体查询与截图；DOM shim 只验证状态流转，不能替代本脚本。
 *
 * 覆盖：
 *   1. file:// 与本地 HTTP 两种打开方式；
 *   2. 1232 / 1020 / 760 / 560 px 四档宽度：无页面横向溢出、顶栏/棋盘
 *      不重叠、球面字母可见、控件可点；
 *   3. 主流程：case03 发射 → 连锁 → 胜利结算 → 撤销；
 *   4. 弹窗：帮助、关卡、变色球选色、REVERSE 区间、失败结算；
 *   5. 键盘：Tab 顺序、弹窗焦点陷阱、Esc 关闭；
 *   6. prefers-reduced-motion：动画即时完成；
 *   7. 色盲可读性：灰度截图中仍保留球面字母（颜色 + 字母双编码）。
 *
 * 依赖（仅本目标需要，不参与 make webverify）：
 *   npm install --no-save playwright
 *   npx playwright install chromium
 *   npx playwright install-deps chromium   # 首次在容器中运行时
 *
 * 运行：
 *   node tools/web_browser_acceptance.js
 *   # 或 make browsercheck
 *
 * 输出：docs/browser-acceptance/*.png 与 report.json；任一检查失败退出 1。
 */
"use strict";

const fs = require("fs");
const http = require("http");
const path = require("path");
const { pathToFileURL } = require("url");

let chromium;
try {
  ({ chromium } = require("playwright"));
} catch (err) {
  console.error("未找到 playwright。请先执行：");
  console.error("  npm install --no-save playwright");
  console.error("  npx playwright install chromium");
  console.error("  npx playwright install-deps chromium");
  process.exit(2);
}

const ROOT = path.resolve(__dirname, "..");
const WEB_INDEX = path.join(ROOT, "web", "index.html");
const OUT = path.join(ROOT, "docs", "browser-acceptance");
const results = [];

function check(cond, label, detail) {
  const ok = !!cond;
  results.push({ ok, label, detail: detail || "" });
  if (ok) {
    console.log("PASS " + label);
  } else {
    console.error("FAIL " + label + (detail ? " — " + detail : ""));
  }
}

function contentType(file) {
  const ext = path.extname(file).toLowerCase();
  return {
    ".html": "text/html; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
    ".json": "application/json; charset=utf-8",
    ".svg": "image/svg+xml",
    ".png": "image/png",
    ".ico": "image/x-icon"
  }[ext] || "application/octet-stream";
}

function startServer() {
  return new Promise((resolve, reject) => {
    const server = http.createServer((req, res) => {
      const urlPath = decodeURIComponent((req.url || "/").split("?")[0]);
      const file = path.normalize(path.join(ROOT, urlPath));
      if (!file.startsWith(ROOT + path.sep) && file !== ROOT) {
        res.statusCode = 403;
        res.end("forbidden");
        return;
      }
      fs.readFile(file, (err, data) => {
        if (err) {
          res.statusCode = 404;
          res.end("not found");
          return;
        }
        res.setHeader("Content-Type", contentType(file));
        res.end(data);
      });
    });
    server.on("error", reject);
    server.listen(0, "127.0.0.1", () => resolve(server));
  });
}

function collectErrors(page, errors, label) {
  page.on("console", (msg) => {
    if (msg.type() === "error") errors.push(label + " console: " + msg.text());
  });
  page.on("pageerror", (err) => errors.push(label + " pageerror: " + err.message));
  page.on("requestfailed", (req) => {
    const reason = req.failure() ? req.failure().errorText : "unknown";
    errors.push(label + " requestfailed: " + req.url() + " (" + reason + ")");
  });
}

async function waitApp(page) {
  await page.waitForFunction(() => {
    return window.Zuma && window.Zuma.app && window.Zuma.app.controller &&
      document.querySelectorAll("#track-balls .track-ball").length > 0;
  }, null, { timeout: 10000 });
  await page.waitForTimeout(150);
}

async function waitIdle(page) {
  await page.waitForFunction(() => {
    return window.Zuma && window.Zuma.app && window.Zuma.app.view &&
      !window.Zuma.app.view.busy;
  }, null, { timeout: 10000 });
  await page.waitForTimeout(100);
}

async function screenshot(page, name) {
  await page.screenshot({ path: path.join(OUT, name), fullPage: false });
}

async function layoutInfo(page) {
  return page.evaluate(() => {
    const de = document.documentElement;
    const rect = (sel) => {
      const el = document.querySelector(sel);
      if (!el) return null;
      const r = el.getBoundingClientRect();
      return {
        left: r.left, right: r.right, top: r.top, bottom: r.bottom,
        width: r.width, height: r.height,
        visible: r.width > 0 && r.height > 0 && !el.hidden
      };
    };
    const balls = Array.from(document.querySelectorAll("#track-balls .track-ball"));
    return {
      vw: de.clientWidth,
      vh: de.clientHeight,
      scrollW: Math.max(de.scrollWidth, document.body.scrollWidth),
      hud: rect(".hud"),
      board: rect("#board"),
      controls: [rect(".controls"), rect(".board-stats"), rect(".launcher-zone")],
      ballCount: balls.length,
      letters: balls.map((b) => {
        const el = b.querySelector(".ball-letter");
        return el ? el.textContent.trim() : "";
      }),
      selected: document.querySelector(".slot.is-selected") ?
        document.querySelector(".slot.is-selected").dataset.pos : null
    };
  });
}

async function checkLayout(page, label) {
  const info = await layoutInfo(page);
  check(info.scrollW <= info.vw + 2, label + " 无页面横向溢出",
    "scrollWidth=" + info.scrollW + " viewport=" + info.vw);
  check(!!info.hud && !!info.board, label + " 顶栏/棋盘存在");
  if (info.hud && info.board) {
    check(info.hud.bottom <= info.board.top + 2, label + " 顶栏与棋盘不重叠",
      "hud.bottom=" + info.hud.bottom + " board.top=" + info.board.top);
    check(info.board.left >= -1 && info.board.right <= info.vw + 1,
      label + " 棋盘位于视口内",
      "board=[" + info.board.left + "," + info.board.right + "]");
  }
  check(info.ballCount > 0, label + " 轨道有球");
  check(info.letters.length === info.ballCount &&
    info.letters.every((x) => x.length > 0),
    label + " 每颗球都有球面字母",
    JSON.stringify(info.letters));
  check(info.controls.every((r) => r && r.visible),
    label + " 控制条/队列区域可见");
  return info;
}

async function checkFocusOrder(page) {
  await page.evaluate(() => document.getElementById("board").focus());
  const seen = [];
  for (let i = 0; i < 22; i++) {
    await page.keyboard.press("Tab");
    seen.push(await page.evaluate(() => {
      const el = document.activeElement;
      if (!el) return "";
      return el.id || el.getAttribute("data-action") || el.tagName.toLowerCase();
    }));
  }
  const required = ["level-button", "history", "sound-button", "pause", "help",
    "undo-button", "hint-button", "swap-button", "reverse-button"];
  required.forEach((key) => {
    check(seen.includes(key), "Tab 顺序覆盖 " + key, seen.join(" → "));
  });
  return seen;
}

async function confirmIfAsked(page) {
  if (await page.locator("#confirm-modal:not([hidden])").count()) {
    await page.click("#confirm-ok");
    await page.waitForSelector("#confirm-modal", { state: "hidden" });
  }
}

async function loadLevel(page, id, label) {
  await waitIdle(page);
  if (await page.locator("#result-modal:not([hidden])").count()) {
    await page.click("#result-levels");
  } else {
    await page.click("#level-button");
  }
  await page.waitForSelector("#level-modal:not([hidden])");
  await page.click('.level-card[data-id="' + id + '"]');
  await confirmIfAsked(page);
  await page.waitForFunction((want) => {
    return document.getElementById("level-label").textContent.indexOf(want) >= 0;
  }, label, { timeout: 5000 });
  await page.waitForTimeout(250);
}

async function engineState(page) {
  return page.evaluate(() => {
    const c = window.Zuma.app.controller;
    const snap = c.snapshot();
    return {
      score: snap.score,
      moves: snap.moves,
      status: snap.status,
      track: snap.track.map((b) => b.color).join(""),
      queue: snap.queue.map((b) => b.color).join(""),
      history: c.history.length
    };
  });
}

async function main() {
  fs.mkdirSync(OUT, { recursive: true });
  const server = await startServer();
  const port = server.address().port;
  const httpUrl = "http://127.0.0.1:" + port + "/web/index.html";
  const fileUrl = pathToFileURL(WEB_INDEX).href;

  const browser = await chromium.launch();
  check(true, "Chromium 启动成功", await browser.version());

  try {
    const errors = [];
    const page = await browser.newPage({ viewport: { width: 1232, height: 900 } });
    collectErrors(page, errors, "file");
    await page.goto(fileUrl);
    await waitApp(page);
    check(await page.title() === "祖玛 · 彩球消除", "file:// 页面标题正确");
    check((await page.evaluate(() => location.protocol)) === "file:",
      "file:// 直接打开成功");

    // 四档布局 + 截图（同一真实页面，仅改视口）
    for (const width of [1232, 1020, 760, 560]) {
      await page.setViewportSize({ width, height: 900 });
      await page.waitForTimeout(300);
      await checkLayout(page, width + "px");
      await screenshot(page, "main-" + width + ".png");
    }

    // 回到桌面宽度；完整 Tab 顺序在载入双球队列后检查（此时撤销/交换可用）
    await page.setViewportSize({ width: 1232, height: 900 });
    await page.waitForTimeout(300);
    let focusOrder = [];

    // 色盲可读性：灰度下仍保留球面字母
    await page.evaluate(() => {
      document.getElementById("track-balls").style.filter = "grayscale(1)";
    });
    await screenshot(page, "grayscale-1232.png");
    const grayLetters = await page.evaluate(() => {
      return Array.from(document.querySelectorAll("#track-balls .ball-letter"))
        .map((el) => el.textContent.trim());
    });
    check(grayLetters.length > 0 && grayLetters.every(Boolean),
      "灰度模式下球面字母仍可读", grayLetters.join(""));
    await page.evaluate(() => {
      document.getElementById("track-balls").style.filter = "";
    });

    // 主流程：case03 发射 → 两轮连锁 → 胜利结算 → 撤销
    await page.click('.slot[data-pos="4"]');
    await page.waitForSelector("#result-modal:not([hidden])", { timeout: 8000 });
    await page.waitForTimeout(300);
    check((await page.textContent("#level-label")).indexOf("03") >= 0,
      "胜利结算发生在 case03 关卡");
    const win = await page.evaluate(() => ({
      score: document.getElementById("result-score").textContent,
      moves: document.getElementById("result-moves").textContent,
      chain: document.getElementById("result-chain").textContent,
      cleared: document.getElementById("result-cleared").textContent,
      title: document.getElementById("result-title").textContent
    }));
    check(win.score === "110" && win.moves === "1" && win.chain === "×2" &&
      win.cleared === "7", "胜利结算数据正确", JSON.stringify(win));
    await screenshot(page, "result-win-1232.png");

    await page.click("#result-undo");
    await page.waitForSelector("#result-modal", { state: "hidden", timeout: 5000 });
    await page.waitForFunction(() => {
      return document.querySelectorAll("#track-balls .track-ball").length === 6;
    }, null, { timeout: 5000 });
    const undone = await engineState(page);
    check(undone.track === "RRBBRR" && undone.queue === "B" &&
      undone.score === 0 && undone.moves === 0,
      "结算页撤销恢复初始局面", JSON.stringify(undone));
    check(await page.evaluate(() => !document.querySelector(".confetti-layer")),
      "撤销关闭结算后彩带已清除");
    await page.waitForTimeout(450); // 等球体重新入场动画结束
    check(await page.evaluate(() => {
      const balls = Array.from(document.querySelectorAll("#track-balls .track-ball"));
      return balls.length === 6 && balls.every((b) => {
        const cs = getComputedStyle(b);
        return cs.visibility !== "hidden" && Number(cs.opacity) > 0.9;
      });
    }), "撤销后 6 颗球均可见");
    await screenshot(page, "undo-1232.png");

    // 帮助弹窗：焦点陷阱 + Esc 关闭
    await page.click('[data-action="help"]');
    await page.waitForSelector("#help-modal:not([hidden])");
    await page.waitForTimeout(300); // 等 modal-pop / fade-in 完成再截图
    await screenshot(page, "help-1232.png");
    let trapped = true;
    for (let i = 0; i < 14; i++) {
      await page.keyboard.press("Tab");
      trapped = trapped && await page.evaluate(() => {
        return document.getElementById("help-modal").contains(document.activeElement);
      });
    }
    check(trapped, "帮助弹窗内 Tab 焦点不逃出");
    await page.keyboard.press("Escape");
    await page.waitForSelector("#help-modal", { state: "hidden" });
    check(await page.evaluate(() =>
      !document.getElementById("help-modal").contains(document.activeElement)),
    "Esc 关闭帮助并离开弹窗");

    // 关卡弹窗 + REVERSE 区间选择
    await loadLevel(page, "case12", "12");
    await page.keyboard.press("r");
    await page.click("#track-balls .track-ball:nth-child(3)");
    await page.click("#track-balls .track-ball:nth-child(6)");
    await page.waitForSelector("#range-bar:not([hidden])");
    await page.waitForTimeout(200);
    await screenshot(page, "reverse-range-1232.png");
    await page.click("#range-confirm");
    await page.waitForFunction(() => {
      return document.querySelectorAll("#track-balls .track-ball").length === 2;
    }, null, { timeout: 6000 });
    const reversed = await engineState(page);
    check(reversed.score === 40 && reversed.track === "BB" && reversed.moves === 1,
      "REVERSE 区间选择与消除正确", JSON.stringify(reversed));
    await screenshot(page, "reverse-result-1232.png");

    // 变色球选色 → 消除 → 队列耗尽判负
    await loadLevel(page, "case09", "09");
    await page.click("#launcher");
    await page.waitForSelector("#color-popover:not([hidden])");
    await page.waitForTimeout(200);
    await screenshot(page, "color-picker-1232.png");
    await page.keyboard.press("1");
    await page.waitForFunction(() => {
      return document.getElementById("score-value").textContent === "30";
    }, null, { timeout: 6000 });
    await waitIdle(page);
    check((await engineState(page)).track === "B", "变色球数字键定色并消除");
    await page.click("#launcher");
    await page.waitForSelector("#result-modal:not([hidden])", { timeout: 6000 });
    check((await page.textContent("#result-title")).indexOf("差一点") >= 0,
      "队列耗尽显示失败结算");
    await page.waitForTimeout(300);
    await screenshot(page, "result-lose-1232.png");

    // SWAP + HINT
    await loadLevel(page, "case10", "10");
    const beforeSwap = await engineState(page);
    await page.click("#swap-button");
    await page.waitForFunction(() => {
      return window.Zuma.app.controller.engine.queue.map((b) => b.color).join("") === "RB";
    }, null, { timeout: 4000 });
    await page.waitForTimeout(400);
    await waitIdle(page);
    check(beforeSwap.queue === "BR", "SWAP 前队首为 B/R", beforeSwap.queue);
    focusOrder = await checkFocusOrder(page);
    await page.click("#hint-button");
    await page.waitForSelector("#hint-badge:not([hidden])", { timeout: 4000 });
    check(true, "HINT 徽标可见");
    await screenshot(page, "hint-1232.png");

    // 炸弹
    await loadLevel(page, "case08", "08");
    await page.click("#launcher");
    await page.waitForFunction(() => {
      return document.getElementById("score-value").textContent === "30";
    }, null, { timeout: 6000 });
    const bomb = await engineState(page);
    check(bomb.track === "B" && bomb.score === 30 && bomb.moves === 1,
      "炸弹邻域消除正确", JSON.stringify(bomb));
    await screenshot(page, "bomb-1232.png");

    // LIMIT 失败结算
    await loadLevel(page, "case13", "13");
    await page.click('.slot[data-pos="0"]');
    await waitIdle(page);
    await page.click('.slot[data-pos="0"]');
    await page.waitForSelector("#result-modal:not([hidden])", { timeout: 6000 });
    check((await page.textContent("#result-sub")).indexOf("步数达到上限") >= 0,
      "LIMIT 达到上限显示失败结算");
    await page.waitForTimeout(300);
    await screenshot(page, "result-limit-1232.png");

    check(errors.length === 0, "file:// 无控制台/资源错误", errors.join(" | "));
    await page.close();

    // HTTP 打开方式
    const httpErrors = [];
    const httpPage = await browser.newPage({ viewport: { width: 1020, height: 900 } });
    collectErrors(httpPage, httpErrors, "http");
    await httpPage.goto(httpUrl);
    await waitApp(httpPage);
    check((await httpPage.evaluate(() => location.protocol)) === "http:",
      "本地 HTTP 打开成功");
    const resources = await httpPage.evaluate(() => {
      return performance.getEntriesByType("resource").map((e) => e.name);
    });
    check(resources.every((u) => u.indexOf(httpUrl.slice(0, -10)) === 0),
      "HTTP 页面资源全部来自本地", resources.join(", "));
    await screenshot(httpPage, "main-http-1020.png");
    check(httpErrors.length === 0, "HTTP 无控制台/资源错误", httpErrors.join(" | "));
    await httpPage.close();

    // prefers-reduced-motion：动画应即时完成
    const rmContext = await browser.newContext({
      viewport: { width: 760, height: 900 },
      reducedMotion: "reduce"
    });
    const rmPage = await rmContext.newPage();
    const rmErrors = [];
    collectErrors(rmPage, rmErrors, "reduced");
    await rmPage.goto(fileUrl);
    await waitApp(rmPage);
    check(await rmPage.evaluate(() =>
      window.matchMedia("(prefers-reduced-motion: reduce)").matches),
    "reduced-motion 媒体查询生效");
    const started = Date.now();
    await rmPage.click('.slot[data-pos="4"]');
    await rmPage.waitForSelector("#result-modal:not([hidden])", { timeout: 3000 });
    const elapsed = Date.now() - started;
    check(elapsed < 1200, "reduced-motion 下结算在 1.2s 内完成",
      elapsed + "ms");
    await screenshot(rmPage, "reduced-result-760.png");
    check(rmErrors.length === 0, "reduced-motion 无控制台错误",
      rmErrors.join(" | "));
    await rmContext.close();

    await browser.close();
    server.close();

    const failed = results.filter((r) => !r.ok);
    fs.writeFileSync(path.join(OUT, "report.json"), JSON.stringify({
      date: new Date().toISOString(),
      browser: "Chromium " + browser.version() + " via Playwright",
      checks: results,
      focusOrder,
      screenshots: fs.readdirSync(OUT).filter((f) => f.endsWith(".png")).sort()
    }, null, 2) + "\n");
    console.log("\n汇总：" + (results.length - failed.length) + " / " +
      results.length + " 项通过；截图与 report.json 位于 " +
      path.relative(ROOT, OUT));
    if (failed.length) {
      console.log("失败项：");
      failed.forEach((f) => console.log("  - " + f.label + " " + f.detail));
      process.exit(1);
    }
  } finally {
    await browser.close().catch(() => {});
    server.close();
  }
}

main().catch((err) => {
  console.error(err && err.stack ? err.stack : err);
  process.exit(1);
});