#!/usr/bin/env node
/*
 * 网页 JS 引擎 / 协议 / 控制器单元测试（阶段 5b）。
 * 运行：node tools/web_engine_test.js（或 make webtest）
 *
 * 覆盖：装载 settle、三连、连锁、炸弹、变色球零副作用、倒退、终局、
 * 提示、撤销还原、随机对局全量撤销、Controller 历史与协议 Session 规则。
 * 与 tools/web_crosscheck.js（C++ 对照）互补：这里断言 JS 侧语义细节。
 */
"use strict";

const assert = require("assert");
const path = require("path");
const fs = require("fs");

const ROOT = path.resolve(__dirname, "..");
const Zuma = require(path.join(ROOT, "web/assets/engine.js"));
const Protocol = require(path.join(ROOT, "web/assets/protocol.js"));
const Ctrl = require(path.join(ROOT, "web/assets/controller.js"));

let passed = 0;
function check(label, fn) {
  try {
    fn();
    passed++;
    console.log("PASS " + label);
  } catch (err) {
    console.error("FAIL " + label);
    console.error(err && err.stack ? err.stack : err);
    process.exit(1);
  }
}
function eq(actual, expected, label) {
  assert.deepStrictEqual(actual, expected, label);
}
function throwsProto(fn, label) {
  assert.throws(fn, (err) => err instanceof Zuma.ProtocolError, label);
}

/* ---------------- 基础规则 ---------------- */

check("装载 settle 消除初始三连", () => {
  const g = new Zuma.Engine({ colors: "RB", track: "RRRBB", queue: "B" });
  eq(g.trackString(), "BB", "track");
  eq(g.queueString(), "B", "queue");
  eq(g.score, 0, "score");
  eq(g.moves, 0, "moves");
  eq(g.initialTotal, 3, "initialTotal");
});

check("三连消除计分并胜利", () => {
  const g = new Zuma.Engine({ colors: "RB", track: "RR", queue: "R" });
  const r = g.shoot(2);
  eq(r.events.length, 1, "event count");
  eq(r.events[0].count, 3, "run");
  eq(r.events[0].points, 30, "points");
  eq(g.trackString(), "", "track");
  eq(g.queueString(), "", "queue");
  eq(g.score, 30, "score");
  eq(g.moves, 1, "moves");
  eq(g.status, "win", "status");
});

check("多级连锁倍率 10 → 20", () => {
  const g = new Zuma.Engine({ colors: "RB", track: "RRBBRR", queue: "B" });
  const r = g.shoot(4);
  eq(r.events.map((e) => e.round), [1, 2], "rounds");
  eq(r.events.map((e) => e.count), [3, 4], "counts");
  eq(r.events.map((e) => e.points), [30, 80], "points");
  eq(g.score, 110, "score");
  eq(g.trackString(), "", "track");
  eq(g.status, "win", "status");
});

check("炸弹右侧优先消除至多三球", () => {
  const g = new Zuma.Engine({ colors: "RGB", track: "RGB", queue: "*R" });
  const r = g.shoot(1);
  eq(r.events.length, 1, "event count");
  eq(r.events[0].count, 3, "run");
  eq(r.events[0].balls, "R*G", "removed balls");
  eq(g.trackString(), "B", "track");
  eq(g.score, 30, "score");
});

check("变色球未指定/非法颜色零副作用", () => {
  const g = new Zuma.Engine({ colors: "RB", track: "RRB", queue: "?R" });
  const beforeTrack = g.trackString();
  throwsProto(() => g.shoot(2), "missing color");
  throwsProto(() => g.shoot(2, "X"), "invalid color");
  eq(g.queueString(), "?R", "queue unchanged");
  eq(g.trackString(), beforeTrack, "track unchanged");
  eq(g.moves, 0, "moves unchanged");
  eq(g.score, 0, "score unchanged");
  g.shoot(2, "R");
  eq(g.trackString(), "B", "after colored shot");
  eq(g.queueString(), "R", "queue after colored shot");
  throwsProto(() => g.shoot(1, "R"), "normal ball with color");
  eq(g.queueString(), "R", "queue still unchanged");
});

check("REVERSE 反转区间后拼出四连", () => {
  const g = new Zuma.Engine({ colors: "RB", track: "RRBBRR", queue: "R" });
  const r = g.reverse(2, 5);
  eq(r.events.length, 1, "event count");
  eq(r.events[0].count, 4, "run");
  eq(r.events[0].points, 40, "points");
  eq(g.trackString(), "BB", "track");
  eq(g.score, 40, "score");
  eq(g.moves, 1, "moves");
  eq(g.status, "playing", "status");
});

check("REVERSE 右拼缝单独成三连（随机差分回归）", () => {
  const g = new Zuma.Engine({ colors: "RBG", track: "RBGBB", queue: "R" });
  const r = g.reverse(1, 2); // R B G B B → R G B B B
  eq(r.events.length, 1, "event count");
  eq(r.events[0].count, 3, "run");
  eq(r.events[0].points, 30, "points");
  eq(g.trackString(), "RG", "track");
  eq(g.score, 30, "score");
  assert.ok(g.undo(), "undo");
  eq(g.trackString(), "RBGBB", "track restored");
  eq(g.score, 0, "score restored");
});

check("LIMIT 达到上限判负", () => {
  const g = new Zuma.Engine({ colors: "RB", track: "RB", queue: "RRR", limit: 2 });
  g.shoot(0);
  eq(g.status, "playing", "after first");
  g.shoot(0);
  eq(g.moves, 2, "moves");
  eq(g.status, "lose", "after limit");
});

check("SWAP 达到 LIMIT 判负且可撤销", () => {
  const g = new Zuma.Engine({ colors: "RB", track: "RB", queue: "RGB", limit: 1 });
  g.swap();
  eq(g.queueString(), "GRB", "swapped");
  eq(g.moves, 1, "moves");
  eq(g.status, "lose", "limit");
  assert.ok(g.undo(), "undo swap");
  eq(g.queueString(), "RGB", "restored");
  eq(g.moves, 0, "moves restored");
  eq(g.status, "playing", "playing restored");
});

check("队列耗尽仍有余球判负", () => {
  const g = new Zuma.Engine({ colors: "RB", track: "RB", queue: "R" });
  g.shoot(0);
  eq(g.queueString(), "", "queue");
  eq(g.trackString(), "RRB", "track");
  eq(g.status, "lose", "status");
});

check("提示选择最左最佳插入位", () => {
  const a = new Zuma.Engine({ colors: "RB", track: "RRB", queue: "R" });
  eq(a.hint(), 0, "leftmost triple");
  const b = new Zuma.Engine({ colors: "RGB", track: "RGB", queue: "*R" });
  eq(b.hint(), 0, "bomb leftmost including endpoint");
  const c = new Zuma.Engine({ colors: "RB", track: "RB", queue: "?R" });
  eq(c.hint(), -1, "chameleon hint");
});

check("发射 / 连锁 / 倒退 / 炸弹撤销还原", () => {
  const cases = [
    { colors: "RB", track: "RRB", queue: "R", op: (g) => g.shoot(2) },
    { colors: "RB", track: "RRBBRR", queue: "B", op: (g) => g.shoot(4) },
    { colors: "RB", track: "RRBBRR", queue: "R", op: (g) => g.reverse(2, 5) },
    { colors: "RGB", track: "RGB", queue: "*R", op: (g) => g.shoot(1) }
  ];
  cases.forEach((c, i) => {
    const g = new Zuma.Engine(c);
    const track = g.trackString();
    const queue = g.queueString();
    c.op(g);
    assert.ok(g.undo(), "undo available " + i);
    eq(g.trackString(), track, "track restored " + i);
    eq(g.queueString(), queue, "queue restored " + i);
    eq(g.score, 0, "score restored " + i);
    eq(g.moves, 0, "moves restored " + i);
    eq(g.status, "playing", "status restored " + i);
    eq(g.undoAvailable, false, "undo stack " + i);
  });
});

check("炸弹 HINT 首尾/单球边界与实际首轮数一致", () => {
  for (const track of ["R", "RG", "RGB"]) {
    const opts = { colors: "RGB", track, queue: "*R" };
    eq(new Zuma.Engine(opts).hint(), 0, "leftmost " + track);
    for (let pos = 0; pos <= track.length; pos++) {
      const g = new Zuma.Engine(opts);
      const result = g.shoot(pos);
      eq(result.events[0].count, Math.min(3, track.length + 1), "count at " + pos);
      assert.ok(g.undo());
      eq(g.trackString(), track);
      eq(g.queueString(), "*R");
    }
  }
});

check("反转两处独立连锁各从第1轮开始", () => {
  const g = new Zuma.Engine({ colors: "RGB", track: "GGRGRGRR", queue: "R" });
  const r = g.reverse(2, 5);
  eq(r.events.map(e => e.round), [1, 1]);
  eq(r.events.map(e => e.points), [30, 30]);
  eq(g.trackString(), "RG");
  eq(g.score, 60);
  g.undo();
  eq(g.trackString(), "GGRGRGRR");
});

check("QUIT 截断、终局 HINT 回显与装载终局 HELP", () => {
  const input = "COLORS RB\nTRACK RB\nQUEUE RRB\n";
  const prefix = "SHOOT 0\nQUIT\n";
  eq(Protocol.runInput(input + prefix).tokens,
     Protocol.runInput(input + prefix + "UNDO\nHELP\nHINT\nQUIT\n").tokens);
  const session = new Protocol.Session({ colors: "RB", track: "RR", queue: "R" });
  session.execute("SHOOT 0");
  const rejected = session.execute("hint");
  eq(rejected.echo, true);
  eq(rejected.tokens, [{ type: "error", reason: "终局后不接受指令" }]);
  const loaded = Protocol.runInput("COLORS RB\nTRACK\nQUEUE R\nHELP\nHINT\nUNDO\n");
  eq(loaded.tokens.filter(t => t.type === "final").length, 1);
  eq(loaded.tokens.filter(t => t.type === "help").length, 1);
  eq(loaded.tokens.filter(t => t.type === "error").length, 2);
  const ended = Protocol.runInput(input + "QUIT\n").session;
  const before = ended.engine.snapshot();
  eq(ended.execute("UNDO").tokens, []);
  eq(ended.engine.snapshot(), before);
  eq(ended.engine.undo(), false, "核心引擎 QUIT 后不可撤销");
  eq(ended.engine.snapshot(), before);
});

/* ---------------- 随机对局全量撤销 ---------------- */

function lcg(seed) {
  let a = seed >>> 0;
  return function () {
    a = (Math.imul(a, 1664525) + 1013904223) >>> 0;
    return a / 4294967296;
  };
}

check("随机对局 0–8 步后全量撤销还原", () => {
  for (let iter = 0; iter < 400; iter++) {
    const rng = lcg(1000 + iter);
    const scenario = Ctrl.randomScenario({
      seed: "undo-" + iter,
      colorCount: 2 + Math.floor(rng() * 5),
      trackLength: 3 + Math.floor(rng() * 18),
      queueLength: 5 + Math.floor(rng() * 14),
      specialRate: 0.12
    });
    const g = new Zuma.Engine(scenario);
    const initial = {
      track: g.trackString(), queue: g.queueString(),
      score: g.score, moves: g.moves, status: g.status
    };
    const steps = Math.floor(rng() * 9);
    for (let k = 0; k < steps; k++) {
      if (g.status !== "playing") break;
      const kind = Math.floor(rng() * 3);
      try {
        if (kind === 0) {
          const pos = Math.floor(rng() * (g.trackSize + 1));
          const front = g.queueBalls[0];
          const color = front && front.color === "?"
            ? scenario.colors.charAt(Math.floor(rng() * scenario.colors.length))
            : null;
          g.shoot(pos, color);
        } else if (kind === 1 && g.queueBalls.length >= 2) {
          g.swap();
        } else if (g.trackSize >= 2) {
          const i = Math.floor(rng() * (g.trackSize - 1));
          const j = i + 1 + Math.floor(rng() * (g.trackSize - i - 1));
          g.reverse(i, j);
        }
      } catch (err) {
        if (!(err instanceof Zuma.ProtocolError)) throw err;
      }
    }
    while (g.undo()) { /* 全部撤销 */ }
    eq(g.trackString(), initial.track, "track iter " + iter);
    eq(g.queueString(), initial.queue, "queue iter " + iter);
    eq(g.score, initial.score, "score iter " + iter);
    eq(g.moves, initial.moves, "moves iter " + iter);
    eq(g.status, initial.status, "status iter " + iter);
    eq(g.undoAvailable, false, "empty undo stack " + iter);
  }
});

/* ---------------- Controller ---------------- */

check("Controller 记录回合、连锁峰值与历史快照", () => {
  const c = new Ctrl.Controller({ colors: "RB", track: "RRBBRR", queue: "B" });
  eq(c.progress(), 0, "initial progress");
  const turn = c.shoot(4);
  eq(c.history.length, 1, "history");
  eq(turn.kind, "shoot", "kind");
  eq(turn.chainPeak, 2, "chain peak");
  eq(turn.scoreDelta, 110, "delta");
  eq(turn.after.status, "win", "after status");
  eq(c.clearedCount(), 7, "cleared");
  eq(c.progress(), 1, "win progress");
  eq(c.snapshotAt(-1).track.map((n) => n.color).join(""), "RRBBRR", "initial snapshot");
  const undo = c.undo();
  eq(undo.kind, "undo", "undo kind");
  eq(c.history.length, 2, "undo recorded");
  eq(c.score, 0, "score after undo");
  eq(c.moves, 0, "moves after undo");
  eq(c.status, "playing", "status after undo");
});

check("Controller 提示不计步数并附带位置", () => {
  const c = new Ctrl.Controller({ colors: "RB", track: "RRB", queue: "R" });
  const turn = c.hint();
  eq(turn.hintPos, 0, "hint pos");
  eq(turn.noMove, true, "no move");
  eq(c.moves, 0, "moves");
  eq(c.history.length, 1, "history");
  eq(c.lastTurn, turn, "lastTurn");
});

check("随机局面同种子可复现且装载不触发三连", () => {
  const opts = { seed: "web-test", colorCount: 4, trackLength: 14,
    queueLength: 14, specialRate: 0.12, limit: 12 };
  const a = Ctrl.randomScenario(opts);
  const b = Ctrl.randomScenario(opts);
  eq(a, b, "same seed");
  const g = new Zuma.Engine(a);
  eq(g.trackString(), a.track, "no initial settle");
  eq(g.status, "playing", "playing");
  eq(Protocol.scenarioToInput(a).indexOf("LIMIT 12") >= 0, true, "limit text");
});

/* ---------------- 深连锁与计分上限（与 C++ 对齐） ---------------- */

// T1 = "BB"，T_{r+1} = XX + T_r + XX（X 在 R/B 间交替）；
// 向中央 BB 对的缝隙（下标 2r-1）插入 B，触发 r 轮消除并清空轨道。
function nestedChainTrack(rounds) {
  let t = "BB";
  let wrap = "R";
  for (let r = 2; r <= rounds; r++) {
    t = wrap + wrap + t + wrap + wrap;
    wrap = wrap === "R" ? "B" : "R";
  }
  return t;
}

check("34 轮深连锁计分与 C++ 一致", () => {
  const rounds = 34;
  const track = nestedChainTrack(rounds);
  eq(track.length, 4 * rounds - 2, "track length");
  const g = new Zuma.Engine({ colors: "RB", track, queue: "B" });
  const r = g.shoot(2 * rounds - 1);
  eq(r.events.length, rounds, "round count");

  let sum = 0;
  let prev = 0;
  r.events.forEach((e, i) => {
    eq(e.round, i + 1, "round " + (i + 1));
    assert.ok(e.points >= 0, "points non-negative");
    assert.ok(e.points <= Number.MAX_SAFE_INTEGER, "points capped");
    sum += e.points;
    assert.ok(sum >= prev, "running total monotonic");
    prev = sum;
  });
  // 手算：首轮 3 球 30 分；第 2..34 轮各 4 球，40×2^(r-1)
  eq(sum, 30 + 40 * (Math.pow(2, rounds) - 2), "hand-computed sum");
  eq(g.score, 687194767310, "win score");
  eq(g.trackString(), "", "track empty");
  eq(g.status, "win", "win");

  g.undo();
  eq(g.trackString(), track, "undo restores 134 balls");
  eq(g.score, 0, "undo score");
  eq(g.moves, 0, "undo moves");
  eq(g.queueString(), "B", "undo queue");
});

check("超深连锁分数饱和于 2^53-1", () => {
  const rounds = 60;
  const g = new Zuma.Engine({
    colors: "RB", track: nestedChainTrack(rounds), queue: "B"
  });
  const r = g.shoot(2 * rounds - 1);
  eq(r.events.length, rounds, "round count");
  let prev = 0;
  r.events.forEach((e) => {
    assert.ok(e.points >= 0, "points non-negative");
    assert.ok(e.points <= Number.MAX_SAFE_INTEGER, "points capped");
    assert.ok(e.points >= prev, "points monotonic");
    prev = e.points;
  });
  eq(g.score, Number.MAX_SAFE_INTEGER, "score saturated");
  eq(g.status, "win", "win");
});

check("WIN 后 swap 拒绝且状态不变", () => {
  const g = new Zuma.Engine({ colors: "RB", track: "RR", queue: "RR" });
  g.shoot(2); // RRR 消空判胜，队列还剩 RR
  eq(g.status, "win", "win first");
  const queueBefore = g.queueString();
  throwsProto(() => g.swap(), "swap after win");
  eq(g.queueString(), queueBefore, "queue unchanged");
  eq(g.score, 30, "score unchanged");
  eq(g.moves, 1, "moves unchanged");
  eq(g.status, "win", "still win");
});

/* ---------------- 协议 Session ---------------- */

check("协议解析拒绝缺失头部", () => {
  eq(Protocol.parseInput("TRACK RB\nQUEUE R\n").ok, false, "no colors");
  eq(Protocol.parseInput("COLORS RB\nQUEUE R\n").ok, false, "no track");
  eq(Protocol.parseInput("COLORS RB\nTRACK RB\n").ok, false, "no queue");
});

check("case09 变色球错误零副作用与最终判负", () => {
  const text = fs.readFileSync(path.join(ROOT, "inputs/case09.txt"), "utf8");
  const res = Protocol.runInput(text);
  eq(res.ok, true, "parse");
  const errors = res.tokens.filter((t) => t.type === "error");
  eq(errors.length, 2, "two errors");
  eq(errors[0].reason, "变色球必须指定 COLORS 内的颜色", "invalid color error");
  eq(errors[1].reason, "仅变色球可指定颜色", "normal ball color error");
  eq(res.session.engine.score, 30, "score");
  eq(res.session.engine.moves, 2, "moves");
  eq(res.session.engine.status, "lose", "status");
});

check("16 个样例均可被 JS 协议执行", () => {
  const dir = path.join(ROOT, "inputs");
  const files = fs.readdirSync(dir).filter((f) => /^case\d+\.txt$/.test(f)).sort();
  eq(files.length, 16, "sample count");
  files.forEach((f) => {
    const res = Protocol.runInput(fs.readFileSync(path.join(dir, f), "utf8"));
    eq(res.ok, true, f);
    assert.ok(res.tokens.length > 0, f + " tokens");
  });
});

console.log("\n汇总：PASS " + passed + " 项");
