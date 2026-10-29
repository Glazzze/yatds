"use strict";
const assert = require("assert");
const fs = require("fs");
const path = require("path");
const os = require("os");
const { execFileSync } = require("child_process");
const { Engine } = require("../web/assets/engine.js");
const { Controller } = require("../web/assets/controller.js");
const ROOT = path.resolve(__dirname, "..");
const js = [];
for (const n of [100, 500, 1000]) {
  const queue = "B".repeat(2000), track = "RGBY".repeat(Math.ceil(n / 4)).slice(0, n);
  const g = new Engine({ colors: "RGBY", track, queue });
  let start = performance.now();
  for (let k = 0; k < 10; k++) assert.equal(g.hint(), -1);
  const hintMeanMs = (performance.now() - start) / 10;
  start = performance.now();
  g.reverse(Math.floor(n / 4), Math.floor(n * 3 / 4));
  const reverseMs = performance.now() - start;
  g.undo();
  assert.equal(g.trackString(), track);

  const rounds = Math.floor((n + 2) / 4);
  let nested = "BB", wrap = "R";
  for (let r = 2; r <= rounds; r++) {
    nested = wrap.repeat(2) + nested + wrap.repeat(2);
    wrap = wrap === "R" ? "B" : "R";
  }
  const prefix = "GY".slice(0, n - nested.length);
  nested = prefix + nested;
  const c = new Controller({ colors: "RGBY", track: nested, queue });
  start = performance.now();
  const turn = c.shoot(prefix.length + 2 * rounds - 1);
  const shootMs = performance.now() - start;
  assert.equal(turn.events.length, rounds);
  const eventSnapshotChars = turn.events.reduce((sum, e) => sum + e.trackAfter.length, 0);
  const eventSnapshotNodes = turn.events.reduce((sum, e) => sum + e.trackSnapshot.length, 0);
  start = performance.now();
  c.undo();
  const undoMs = performance.now() - start;
  assert.equal(c.engine.trackString(), nested);
  assert.equal(c.engine.queueString(), queue);
  assert.equal(c.score, 0);
  assert.equal(c.moves, 0);
  const historyJsonBytes = Buffer.byteLength(JSON.stringify(c.history));
  js.push({ n, queue: 2000, rounds, hintMeanMs, reverseMs, shootMs, undoMs,
    eventSnapshotChars, eventSnapshotNodes, historyEntries: c.history.length, historyJsonBytes });
}
const cpp = JSON.parse(execFileSync(path.join(ROOT, "tools/scale_probe"), { encoding: "utf8" }));
for (let i = 0; i < js.length; i++) {
  assert.equal(cpp[i].eventSnapshotChars, js[i].eventSnapshotChars);
  assert.equal(cpp[i].rounds, js[i].rounds);
}
const result = {
  date: "2026-10-29", environment: { platform: os.platform(), arch: os.arch(), node: process.version },
  method: "C++ 默认 Makefile 无优化构建；JS 为 Node。HINT 10次均值，其余单次；不计编译、进程启动和 DOM 动画。JS shoot/undo 包含 Controller 历史快照。JSON字节仅为序列化量，不是堆占用；不设耗时通过阈值。",
  cpp, js
};
const dest = path.join(ROOT, "docs/scale-observation.json");
fs.writeFileSync(dest, JSON.stringify(result, null, 2) + "\n");
console.log(JSON.stringify(result, null, 2));
console.log("PASS 规模观测：两端连锁轮次、快照字符规模、撤销还原一致；写入 docs/scale-observation.json");
