#!/usr/bin/env node
/*
 * T5.6 双实现交叉验证：JS 引擎 vs C++ CLI（--plain）。
 *
 * 对 inputs/*.txt 逐例运行两套实现，把 C++ 纯协议输出解析为语义 token
 * （header / elim / track / queue / error / hint / help / final），与
 * web/assets/protocol.js 的 Session token 序列逐项比较：
 *   - 消除轮次、left/count/balls/points 与每轮 trackAfter；
 *   - 每步 TRACK/QUEUE 快照、ERROR 原因、HINT 结果；
 *   - 终局状态/分数/步数（含 EOF 的 QUIT_SCORE）。
 *
 * 任一差异打印首个不一致并退出 1。Node ≥18，无需第三方依赖。
 */
"use strict";

const fs = require("fs");
const path = require("path");
const { spawnSync } = require("child_process");

const PROJECT = path.resolve(__dirname, "..");
const Zuma = require(path.join(PROJECT, "web/assets/engine.js"));
const Protocol = require(path.join(PROJECT, "web/assets/protocol.js"));

function parseTrackValue(line) {
  if (line === "TRACK") return "";
  return line.startsWith("TRACK ") ? line.slice(6) : null;
}

function parseQueueValue(line) {
  if (line === "QUEUE") return "";
  return line.startsWith("QUEUE ") ? line.slice(6) : null;
}

function parseElim(line) {
  const m = /^ELIM round=(\d+) left=(\d+) count=(\d+) balls=(\S*) score=\+(\d+)$/
    .exec(line);
  if (!m) return null;
  return {
    type: "elim",
    round: Number(m[1]),
    left: Number(m[2]),
    count: Number(m[3]),
    balls: m[4],
    points: Number(m[5]),
    trackAfter: null
  };
}

function parseFinal(line) {
  let m = /^(WIN|LOSE) SCORE=(-?\d+) MOVES=(-?\d+)$/.exec(line);
  if (m) {
    return { type: "final", status: m[1].toLowerCase(),
             score: Number(m[2]), moves: Number(m[3]) };
  }
  m = /^QUIT_SCORE SCORE=(-?\d+) MOVES=(-?\d+)$/.exec(line);
  if (m) {
    return { type: "final", status: "quit",
             score: Number(m[1]), moves: Number(m[2]) };
  }
  return null;
}

function fail(msg) {
  throw new Error(msg);
}

/* 把 C++ 纯协议输出解析成语义 token；命令列表来自同一输入文件。 */
function parseCppOutput(output, parsed) {
  const lines = output.replace(/\r\n/g, "\n").split("\n");
  if (lines.length && lines[lines.length - 1] === "") lines.pop();
  const tokens = [];
  let i = 0;

  function need(prefix) {
    if (i >= lines.length || !lines[i].startsWith(prefix))
      fail("期望 " + prefix + " 行，实际：" +
           (i < lines.length ? JSON.stringify(lines[i]) : "<EOF>"));
    return lines[i++];
  }

  // 头部：COLORS/TRACK/QUEUE/[LIMIT]/READY
  let sawReady = false;
  while (i < lines.length && !sawReady) {
    const line = lines[i++];
    if (line.startsWith("COLORS ")) {
      tokens.push({ type: "colors", value: line.slice(7) });
    } else if (line.startsWith("TRACK")) {
      tokens.push({ type: "track", value: parseTrackValue(line) });
    } else if (line.startsWith("QUEUE")) {
      tokens.push({ type: "queue", value: parseQueueValue(line) });
    } else if (line.startsWith("LIMIT ")) {
      tokens.push({ type: "limit", value: Number(line.slice(6)) });
    } else if (line === "READY") {
      tokens.push({ type: "ready" });
      sawReady = true;
    } else {
      fail("头部出现未知行：" + JSON.stringify(line));
    }
  }
  if (!sawReady) fail("输出缺少 READY");

  // 装载终局后仍可能有 HELP/UNDO/QUIT，继续按命令解析。
  if (i < lines.length && parseFinal(lines[i])) {
    tokens.push(parseFinal(lines[i++]));

  }

  const commands = parsed.commands;
  for (const raw of commands) {
    const cmd = Protocol.parseCommand(raw);
    if (cmd.kind === "comment") continue;

    if (cmd.kind === "quit") {
      const fin = parseFinal(lines[i] || "");
      if (!fin || fin.status !== "quit")
        fail("QUIT 后缺少 QUIT_SCORE：" + JSON.stringify(lines[i]));
      tokens.push(fin);
      i++;
      break; // QUIT 后剩余输入不应产生输出
    }

    // PLAYING 的合法 HINT 以结果行代替回显；终局拒绝的 HINT 仍回显。
    const sawEcho = i < lines.length && lines[i] === raw;
    if (sawEcho) i++;
    if (cmd.kind !== "hint" && !sawEcho)
      fail("缺少指令回显：" + raw);

    if (i < lines.length && lines[i].startsWith("ERROR ")) {
      if (!sawEcho) fail("错误行前缺少指令回显：" + raw);
      tokens.push({ type: "error", reason: lines[i].slice(6) });
      i++;
      continue;
    }

    if (cmd.kind === "hint") {
      if (sawEcho) fail("合法 HINT 不应额外回显：" + raw);
      const m = /^HINT (-?\d+)$/.exec(lines[i] || "");
      if (!m) fail("期望 HINT 结果行，实际：" + JSON.stringify(lines[i]));
      tokens.push({ type: "hint", value: Number(m[1]) });
      i++;
    } else if (cmd.kind === "help") {
      // HINT 之外的回显已消费；帮助正文固定，消费到末行。
      let ended = false;
      while (i < lines.length) {
        const line = lines[i++];
        if (line.startsWith("位置：")) { ended = true; break; }
      }
      if (!ended) fail("HELP 正文不完整");
      tokens.push({ type: "help" });
    } else if (cmd.kind === "shoot" || cmd.kind === "reverse") {
      let eventCount = 0;
      while (i < lines.length && lines[i].startsWith("ELIM ")) {
        const ev = parseElim(lines[i++]);
        if (!ev) fail("ELIM 格式错误");
        const trackLine = need("TRACK");
        ev.trackAfter = parseTrackValue(trackLine);
        tokens.push(ev);
        eventCount++;
      }
      if (eventCount === 0) {
        const trackLine = need("TRACK");
        tokens.push({ type: "track", value: parseTrackValue(trackLine) });
      }
      if (cmd.kind === "shoot") {
        const queueLine = need("QUEUE");
        tokens.push({ type: "queue", value: parseQueueValue(queueLine) });
      }
    } else if (cmd.kind === "undo") {
      const trackLine = need("TRACK");
      tokens.push({ type: "track", value: parseTrackValue(trackLine) });
      const queueLine = need("QUEUE");
      tokens.push({ type: "queue", value: parseQueueValue(queueLine) });
    } else if (cmd.kind === "swap") {
      const queueLine = need("QUEUE");
      tokens.push({ type: "queue", value: parseQueueValue(queueLine) });
    } else {
      fail("指令应产生 ERROR 但输出中没有：" + raw);
    }

    if (i < lines.length) {
      const fin = parseFinal(lines[i]);
      // 非 QUIT 指令后只可能补 WIN/LOSE；QUIT_SCORE 属于 QUIT 或 EOF。
      if (fin && fin.status !== "quit") { tokens.push(fin); i++; }
    }
  }

  // EOF 收尾：未终局时 C++ 补一条 QUIT_SCORE。
  if (i < lines.length) {
    const fin = parseFinal(lines[i]);
    if (fin) { tokens.push(fin); i++; }
  }
  if (i !== lines.length)
    fail("输出有未消费行：" + JSON.stringify(lines.slice(i, i + 5)));
  return tokens;
}

function compareTokens(name, jsTokens, cppTokens) {
  const n = Math.max(jsTokens.length, cppTokens.length);
  for (let i = 0; i < n; i++) {
    const a = jsTokens[i];
    const b = cppTokens[i];
    if (JSON.stringify(a) !== JSON.stringify(b)) {
      console.error("FAIL " + name + " 首个差异 token #" + i);
      console.error("  JS :", JSON.stringify(a));
      console.error("  C++:", JSON.stringify(b));
      return false;
    }
  }
  return true;
}

function main() {
  const inputDir = path.join(PROJECT, "inputs");
  const files = fs.readdirSync(inputDir).filter(f => f.endsWith(".txt")).sort();
  if (files.length === 0) {
    console.error("inputs/ 下没有样例");
    process.exit(2);
  }

  let passed = 0;
  for (const file of files) {
    const text = fs.readFileSync(path.join(inputDir, file), "utf8");
    const parsed = Protocol.parseInput(text);
    if (!parsed.ok) {
      console.error("FAIL " + file + "：JS 解析输入失败：" + parsed.error);
      continue;
    }

    const proc = spawnSync(path.join(PROJECT, "zuma"),
      ["--plain"], {
        cwd: PROJECT,
        input: text,
        encoding: "utf8",
        env: Object.assign({}, process.env, { NO_COLOR: "1" })
      });
    if (proc.error || proc.status !== 0) {
      console.error("FAIL " + file + "：zuma 运行失败（status=" +
                    proc.status + "）" + (proc.stderr || ""));
      continue;
    }

    let cppTokens, jsTokens;
    try {
      cppTokens = parseCppOutput(proc.stdout, parsed);
    } catch (e) {
      console.error("FAIL " + file + "：解析 C++ 输出失败：" + e.message);
      continue;
    }
    const js = Protocol.runInput(text);
    jsTokens = js.tokens;
    if (!compareTokens(file, jsTokens, cppTokens)) continue;
    console.log("PASS " + file + "（" + jsTokens.length + " tokens）");
    passed++;
  }

  console.log("\n汇总：PASS " + passed + " / " + files.length);
  if (passed !== files.length) process.exit(1);
}

if (require.main === module) main();
module.exports = { parseCppOutput, compareTokens };