/*
 * 祖玛 · 网页控制器（阶段 5b）
 *
 * 职责：包住 Engine，记录每一步动作的前后快照与展示信息（操作历史、
 * 分数变化、连锁峰值），并提供随机局面生成。视图层只读快照，不直接
 * 修改引擎；所有状态变化都经过这里，便于撤销链可视化与测试。
 */
(function (root, factory) {
  if (typeof module === "object" && module.exports)
    module.exports = factory(require("./engine.js"), require("./protocol.js"));
  else {
    root.Zuma = root.Zuma || {};
    Object.assign(root.Zuma, factory(root.Zuma, root.Zuma));
  }
})(typeof globalThis !== "undefined" ? globalThis : this, function (Zuma, Protocol) {
  "use strict";

  function cloneSnapshot(s) {
    return {
      track: s.track.map(function (n) { return { id: n.id, color: n.color }; }),
      queue: s.queue.map(function (n) { return { id: n.id, color: n.color }; }),
      score: s.score,
      moves: s.moves,
      status: s.status,
      limit: s.limit,
      colors: s.colors,
      initialTotal: s.initialTotal,
      undoAvailable: s.undoAvailable
    };
  }

  function hashSeed(seed) {
    var s = String(seed);
    var h = 2166136261 >>> 0;
    for (var i = 0; i < s.length; i++) {
      h ^= s.charCodeAt(i);
      h = Math.imul(h, 16777619) >>> 0;
    }
    return h >>> 0;
  }

  function mulberry32(a) {
    return function () {
      a |= 0;
      a = a + 0x6D2B79F5 | 0;
      var t = Math.imul(a ^ a >>> 15, 1 | a);
      t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t;
      return ((t ^ t >>> 14) >>> 0) / 4294967296;
    };
  }

  class Controller {
    constructor(scenario, meta) {
      this.scenario = {
        colors: scenario.colors,
        track: scenario.track,
        queue: scenario.queue,
        limit: scenario.limit || 0
      };
      this.meta = meta || {};
      this.engine = new Zuma.Engine(this.scenario);
      this.initialSnapshot = cloneSnapshot(this.engine.snapshot());
      this.history = [];
      this.seq = 0;
      this.chainPeak = 0;
      this.lastTurn = null;
    }

    get game() { return this.engine; }
    get status() { return this.engine.status; }
    get score() { return this.engine.score; }
    get moves() { return this.engine.moves; }

    snapshot() { return cloneSnapshot(this.engine.snapshot()); }

    progress() {
      var total = this.engine.initialTotal;
      if (total <= 0) return 1;
      var remain = this.engine.trackSize + this.engine.queueBalls.length;
      var p = 1 - remain / total;
      return Math.max(0, Math.min(1, p));
    }

    clearedCount() {
      var total = this.engine.initialTotal;
      var remain = this.engine.trackSize + this.engine.queueBalls.length;
      return Math.max(0, total - remain);
    }

    _record(turn) {
      turn.seq = ++this.seq;
      this.history.push(turn);
      this.lastTurn = turn;
      if (turn.chainPeak > this.chainPeak) this.chainPeak = turn.chainPeak;
      return turn;
    }

    _makeTurn(kind, label, detail, before, result, noMove) {
      var after = this.snapshot();
      var events = result && result.events ? result.events : [];
      var peak = 0;
      var eliminated = 0;
      events.forEach(function (ev) {
        if (ev.round > peak) peak = ev.round;
        eliminated += ev.count;
      });
      return {
        kind: kind,
        label: label,
        detail: detail || "",
        before: before,
        after: after,
        events: events,
        preSettle: result ? result.preSettle || null : null,
        insertedId: result ? result.insertedId || null : null,
        noMove: !!noMove,
        chainPeak: peak,
        eliminated: eliminated,
        scoreDelta: after.score - before.score
      };
    }

    shoot(pos, color) {
      var before = this.snapshot();
      var front = this.engine.queue[0];
      var frontColor = front ? front.color : "";
      var result = this.engine.shoot(pos, color);
      var label = "发射 " + (frontColor === "?" ? color : frontColor) +
                  " → " + pos;
      if (frontColor === "?") label = "变色 " + frontColor + "→" + color +
                                      " → " + pos;
      return this._record(this._makeTurn("shoot", label, "", before, result));
    }

    swap() {
      var before = this.snapshot();
      var result = this.engine.swap();
      return this._record(this._makeTurn("swap", "交换队首两球", "", before,
                                         result));
    }

    reverse(i, j) {
      var before = this.snapshot();
      var result = this.engine.reverse(i, j);
      return this._record(this._makeTurn("reverse",
        "倒退 [" + i + ", " + j + "]", "", before, result));
    }

    hint() {
      var before = this.snapshot();
      var pos = this.engine.hint();
      var label = pos >= 0 ? "提示 → " + pos : "提示：无可消除位";
      var turn = this._record(this._makeTurn("hint", label, "", before, null, true));
      turn.hintPos = pos;
      return turn;
    }

    undo() {
      var before = this.snapshot();
      if (!this.engine.undo()) return null;
      return this._record(this._makeTurn("undo", "撤销上一步", "", before, null,
                                         true));
    }

    /* 判断某个历史回合后的局面（只读预览用）。 */
    snapshotAt(index) {
      if (index < 0) return this.initialSnapshot;
      if (index >= this.history.length) return this.snapshot();
      return this.history[index].after;
    }
  }

  /* ---------------- 随机局面（仅网页侧，种子可复现） ---------------- */

  var PALETTE = "RGBYMCWK";

  function makeTrack(rng, colors, length) {
    var out = "";
    while (out.length < length) {
      var c = colors.charAt(Math.floor(rng() * colors.length));
      var n = out.length;
      if (n >= 2 && out.charAt(n - 1) === c && out.charAt(n - 2) === c)
        continue; // 避免装载即触发消除
      out += c;
    }
    return out;
  }

  function makeQueue(rng, colors, length, specialRate) {
    var out = "";
    for (var i = 0; i < length; i++) {
      var x = rng();
      if (x < specialRate / 2) out += "*";
      else if (x < specialRate) out += "?";
      else out += colors.charAt(Math.floor(rng() * colors.length));
    }
    return out;
  }

  function randomScenario(options) {
    options = options || {};
    var seed = options.seed === undefined ? String(Date.now()) : String(options.seed);
    var rng = mulberry32(hashSeed(seed));

    var colorCount = Math.max(2, Math.min(6, Number(options.colorCount) || 4));
    var trackLen = Math.max(3, Math.min(30, Number(options.trackLength) || 12));
    var queueLen = Math.max(5, Math.min(30, Number(options.queueLength) || 12));
    var special = Math.max(0, Math.min(0.2,
      options.specialRate === undefined ? 0.12 : Number(options.specialRate)));

    var pool = PALETTE.split("");
    // 种子洗牌选色，保持字母与色板的固定映射
    for (var i = pool.length - 1; i > 0; i--) {
      var j = Math.floor(rng() * (i + 1));
      var t = pool[i]; pool[i] = pool[j]; pool[j] = t;
    }
    var colors = pool.slice(0, colorCount).join("");

    var track = makeTrack(rng, colors, trackLen);
    var queue = makeQueue(rng, colors, queueLen, special);
    var limit = options.limit && Number(options.limit) > 0
      ? Number(options.limit) : 0;
    return {
      seed: seed,
      colors: colors,
      track: track,
      queue: queue,
      limit: limit
    };
  }

  function scenarioText(scenario) {
    return Protocol.scenarioToInput(scenario);
  }

  return {
    Controller: Controller,
    randomScenario: randomScenario,
    scenarioText: scenarioText,
    hashSeed: hashSeed,
    mulberry32: mulberry32
  };
});