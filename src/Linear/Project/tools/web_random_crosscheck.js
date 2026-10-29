"use strict";

// 相同随机指令同时驱动 C++ 与 JS；头部错误另外核对退出码与错误短语。
const assert = require("assert");
const path = require("path");
const { spawnSync } = require("child_process");
const Protocol = require("../web/assets/protocol.js");
const { Engine } = require("../web/assets/engine.js");
const { parseCppOutput } = require("./web_crosscheck.js");
const ROOT = path.resolve(__dirname, "..");
let seed = 20261029;
const random = n => {
  seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
  return seed % n;
};
const driver = input => spawnSync(path.join(ROOT, "zuma"), ["--plain"], {
  input, encoding: "utf8", env: { ...process.env, NO_COLOR: "1" }
});

let boundaryCount = 0;
const header = (colors, track = "RB", queue = "R") =>
  `COLORS ${colors}\nTRACK ${track}\nQUEUE ${queue}\n`;
const boundaries = [
  ["one-color", header("R", "R"), "颜色数量必须为 2–8 种"],
  ["nine-colors", header("ABCDEFGHI", "AB", "A"), "颜色数量必须为 2–8 种"],
  ["track-over", header("RB", "RB".repeat(500) + "R"),
    "初始轨道最多 1000 球，队列最多 2000 球"],
  ["queue-over", header("RB", "RB", "R".repeat(2001)),
    "初始轨道最多 1000 球，队列最多 2000 球"],
  ["max-accepted", header("RB", "RB".repeat(500), "R".repeat(2000)), null],
  ["eight-colors", header("ABCDEFGH", "AB", "A"), null],
  ["empty-track-queue", header("RB", "", ""), null],
  ["duplicate", header("RR", "R"), "颜色集合含重复字母"],
  ["invalid-track", header("RB", "R*"), "轨道含非法球符号"],
  ["invalid-queue", header("RB", "RB", "X"), "待发射队列含非法球符号"],
  ["missing-header", "COLORS RB\nTRACK R\n", "头部不完整（缺 COLORS/TRACK/QUEUE 行或提前 EOF）"],
  ["invalid-limit", header("RB") + "LIMIT 0\n", "LIMIT 行参数不是正整数"]
];
for (const [label, input, error] of boundaries) {
  const js = Protocol.parseInput(input), cpp = driver(input);
  if (error) {
    assert.equal(js.ok, false, label);
    assert.equal(js.error, error, label);
    assert.equal(cpp.status, 1, label);
    assert.equal(cpp.stdout, "", label);
    assert.equal(cpp.stderr.trim(), "输入错误: " + error, label);
  } else {
    assert.equal(js.ok, true, label);
    assert.equal(cpp.status, 0, label);
    assert.deepStrictEqual(parseCppOutput(cpp.stdout, js), Protocol.runInput(input).tokens, label);
  }
  boundaryCount++;
}

function crosscheck(input, label) {
  const cpp = driver(input);
  assert.equal(cpp.status, 0, label + ": " + cpp.stderr);
  assert.deepStrictEqual(parseCppOutput(cpp.stdout, Protocol.parseInput(input)),
    Protocol.runInput(input).tokens, label + "\n" + input);
}

const terminalCases = [
  header("RB", "", "R") + "HELP\nHINT\nUNDO\nQUIT\nHELP\n",
  header("RB", "RB", "") + "HINT\nHELP\nUNDO\nQUIT\n",
  header("RB", "RR", "R") + "SHOOT 0\nHINT\nHELP\nUNDO\nQUIT\nUNDO\n",
  header("RB", "RB", "R") + "SHOOT 0\nHINT\nHELP\nUNDO\nQUIT\n",
  header("RGB", "GGRGRGRR", "R") + "REVERSE 2 5\nUNDO\nQUIT\n"
];
terminalCases.forEach((input, i) => crosscheck(input, "terminal-" + i));

for (let game = 0; game < 500; game++) {
  const colors = "RGBY".slice(0, 2 + random(3));
  const balls = (len, palette) => Array.from({ length: len }, () => palette[random(palette.length)]).join("");
  const scenario = { colors, track: balls(1 + random(18), colors),
    queue: balls(2 + random(18), colors + "*?"), limit: random(4) === 0 ? 1 + random(10) : 0 };
  let input = Protocol.scenarioToInput(scenario);
  const g = new Engine(scenario);
  for (let step = 0; step < 20; step++) {
    const kind = random(8);
    let command;
    if (kind < 3) {
      const pos = random(g.trackSize + 2); // 含一个越界位置
      const color = g.queueString()[0] === "?" && random(5) !== 0 ? colors[random(colors.length)] : null;
      command = "SHOOT " + pos + (color ? " " + color : "");
      try { g.shoot(pos, color); } catch (e) { if (e.name !== "ProtocolError") throw e; }
    } else if (kind === 3) {
      command = "SWAP";
      try { g.swap(); } catch (e) { if (e.name !== "ProtocolError") throw e; }
    } else if (kind === 4) {
      const i = random(g.trackSize + 1), j = i + random(g.trackSize + 1 - i);
      command = `REVERSE ${i} ${j}`;
      try { g.reverse(i, j); } catch (e) { if (e.name !== "ProtocolError") throw e; }
    } else if (kind === 5) { command = "UNDO"; g.undo(); }
    else if (kind === 6) command = "HINT";
    else command = "HELP";
    input += command + "\n";
  }
  // 有一半显式退出，QUIT 后故意追加命令；另一半验证 EOF。
  if (game % 2 === 0) input += "QUIT\nUNDO\nHELP\nQUIT\n";
  crosscheck(input, "seed=20261029 game=" + game);
}
console.log(`PASS 头部边界 ${boundaryCount} 项、终局专项 ${terminalCases.length} 局、跨语言随机 500 局（每局20步，seed=20261029）`);
