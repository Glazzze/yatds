/*
 * 祖玛 · 网页规则引擎（阶段 5b，D7=A 纯前端 JS 复刻）
 *
 * 本文件是 C++ core 的独立 JavaScript 实现，语义以 docs/PROTOCOL.md 与
 * src/core/game.cpp 为唯一规范。引擎不引用 DOM、不依赖浏览器 API，采用
 * UMD 包装：浏览器挂到 window.Zuma.Engine，Node 下 module.exports，
 * 供 tools/web_crosscheck.js 直接 require 做双实现交叉验证。
 *
 * 内部结构与 C++ 对齐：双向链表轨道（节点带稳定 id 供动画定位）、
 * 待发射队列、撤销记录（多轮消除区间逆序回填）。计分、炸弹首轮邻域、
 * 变色定色、终局优先级、装载 settle 全部按 core 实现。
 */
(function (root, factory) {
  if (typeof module === "object" && module.exports)
    module.exports = factory();
  else {
    root.Zuma = root.Zuma || {};
    Object.assign(root.Zuma, factory());
  }
})(typeof globalThis !== "undefined" ? globalThis : this, function () {
  "use strict";

  var BOMB = "*";
  var CHAMELEON = "?";

  /*
   * 分数上限 2^53-1（Number.MAX_SAFE_INTEGER），与 C++ Game::kScoreCap
   * 一致。每个整数 ≤ 2^53-1 时 Number 运算精确；两端在每轮得分与总分
   * 累加上都饱和到该值，保证深连锁下分数仍然逐字节一致。
   */
  var SCORE_CAP = Number.MAX_SAFE_INTEGER;

  function satMul(a, b) {
    if (a <= 0 || b <= 0) return 0;
    if (a > Math.floor(SCORE_CAP / b)) return SCORE_CAP;
    return a * b;
  }

  function satAdd(a, b) {
    if (b >= SCORE_CAP - a) return SCORE_CAP;
    return a + b;
  }

  function elimPoints(count, round) {
    var base = satMul(count, 10);
    if (round <= 1) return base;
    if (round - 1 >= 53) return SCORE_CAP;
    return satMul(base, Math.pow(2, round - 1));
  }

  class ProtocolError extends Error {
    constructor(message) {
      super(message);
      this.name = "ProtocolError";
    }
  }

  class Engine {
    constructor(options) {
      options = options || {};
      this.colors = String(options.colors || "");
      this.limit = Number(options.limit) > 0 ? Number(options.limit) : 0;

      this.head = null;
      this.tail = null;
      this.size = 0;
      this.nextId = 1;

      // 队列元素保留原始符号与稳定 id；变色球发射后 id 仍用于撤销/动画。
      this.queueBalls = [];
      this.score = 0;
      this.moves = 0;
      this.status = "playing";
      this.undoStack = [];

      for (var ch of String(options.track || "")) this._appendNode(ch);
      for (var q of String(options.queue || "")) {
        this.queueBalls.push({ id: this.nextId++, color: q });
      }

      this._initialSettle();
      // 进度分母：装载完成（含初始 settle）后的轨道 + 队列球数。
      this.initialTotal = this.size + this.queueBalls.length;
      this._refreshStatus();
      this.initialStatus = this.status;
    }

    /* ---------------- 只读查询 ---------------- */

    get trackNodes() {
      var out = [];
      for (var p = this.head; p; p = p.next)
        out.push({ id: p.id, color: p.color });
      return out;
    }

    get queue() {
      return this.queueBalls.map(function (b) {
        return { id: b.id, color: b.color };
      });
    }

    get trackSize() {
      return this.size;
    }

    get undoAvailable() {
      return this.undoStack.length > 0;
    }

    trackString() {
      var s = "";
      for (var p = this.head; p; p = p.next) s += p.color;
      return s;
    }

    queueString() {
      return this.queueBalls.map(function (b) { return b.color; }).join("");
    }

    snapshot() {
      return {
        track: this.trackNodes,
        queue: this.queue,
        score: this.score,
        moves: this.moves,
        status: this.status,
        limit: this.limit,
        colors: this.colors,
        initialTotal: this.initialTotal,
        undoAvailable: this.undoAvailable
      };
    }

    /* ---------------- 轨道基本操作 ---------------- */

    _newNode(color) {
      return {
        id: this.nextId++,
        color: color,
        prev: null,
        next: null,
        alive: true
      };
    }

    _appendNode(color) {
      var node = this._newNode(color);
      node.prev = this.tail;
      if (this.tail) this.tail.next = node;
      else this.head = node;
      this.tail = node;
      this.size++;
      return node;
    }

    at(pos) {
      if (pos < 0 || pos >= this.size)
        throw new ProtocolError("下标越界");
      if (pos < this.size - pos) {
        var cur = this.head;
        for (var i = 0; i < pos; i++) cur = cur.next;
        return cur;
      }
      var back = this.tail;
      for (var j = this.size - 1; j > pos; j--) back = back.prev;
      return back;
    }

    _insertAt(pos, color) {
      if (pos < 0 || pos > this.size)
        throw new ProtocolError("发射位置越界");
      var node = this._newNode(color);
      if (pos === 0) {
        node.next = this.head;
        if (this.head) this.head.prev = node;
        this.head = node;
        if (!this.tail) this.tail = node;
      } else if (pos === this.size) {
        node.prev = this.tail;
        if (this.tail) this.tail.next = node;
        this.tail = node;
        if (!this.head) this.head = node;
      } else {
        var succ = this.at(pos);
        node.prev = succ.prev;
        node.next = succ;
        if (succ.prev) succ.prev.next = node;
        else this.head = node;
        succ.prev = node;
      }
      this.size++;
      return node;
    }

    _insertBefore(anchor, balls) {
      var first = null;
      for (var i = 0; i < balls.length; i++) {
        var node = this._newNode(balls[i].color);
        if (!anchor) {
          node.prev = this.tail;
          if (this.tail) this.tail.next = node;
          else this.head = node;
          this.tail = node;
        } else {
          node.prev = anchor.prev;
          node.next = anchor;
          if (anchor.prev) anchor.prev.next = node;
          else this.head = node;
          anchor.prev = node;
        }
        if (!first) first = node;
        this.size++;
      }
      return first;
    }

    _eraseNode(node) {
      if (!node || !node.alive) return;
      if (node.prev) node.prev.next = node.next;
      else this.head = node.next;
      if (node.next) node.next.prev = node.prev;
      else this.tail = node.prev;
      node.alive = false;
      this.size--;
    }

    _eraseRun(left, len) {
      var before = left.prev;
      var cur = left;
      for (var i = 0; i < len; i++) {
        var next = cur.next;
        cur.alive = false;
        cur = next;
      }
      this.size -= len;
      if (before) before.next = cur;
      else this.head = cur;
      if (cur) cur.prev = before;
      else this.tail = before;
      return cur;
    }

    _indexOf(node) {
      var idx = 0;
      for (var p = this.head; p; p = p.next, idx++)
        if (p === node) return idx;
      throw new ProtocolError("节点不属于本轨道");
    }

    _reverseRange(i, j) {
      if (i > j || j >= this.size) throw new ProtocolError("反转区间越界");
      var nodes = [];
      // 下标只定位一次，然后沿 next 收集；定位 O(n)，遍历/重连 O(L)。
      var cur = this.at(i);
      var before = cur.prev;
      for (var k = i; k <= j; k++) {
        nodes.push(cur);
        cur = cur.next;
      }
      var after = cur;
      nodes.reverse(); // 区间节点逆序后重新连接
      for (var n = 0; n < nodes.length; n++) {
        nodes[n].prev = n > 0 ? nodes[n - 1] : before;
        nodes[n].next = n + 1 < nodes.length ? nodes[n + 1] : after;
      }
      if (before) before.next = nodes[0];
      else this.head = nodes[0];
      if (after) after.prev = nodes[nodes.length - 1];
      else this.tail = nodes[nodes.length - 1];
    }

    _findColorRun(node) {
      var left = node;
      var right = node;
      while (left.prev && left.prev.color === node.color) left = left.prev;
      while (right.next && right.next.color === node.color) right = right.next;
      var len = 0;
      for (var p = left; ; p = p.next) {
        len++;
        if (p === right) break;
      }
      return { left: left, right: right, len: len };
    }

    /* ---------------- 装载 settle ---------------- */

    _initialSettle() {
      var cur = this.head;
      while (cur) {
        var run = this._findColorRun(cur);
        if (run.len >= 3) {
          var after = run.right.next;
          this._eraseRun(run.left, run.len);
          cur = after;
        } else {
          cur = cur.next;
        }
      }
    }

    /* ---------------- 消除与连锁 ---------------- */

    _eliminateRun(left, right, len, round, events, entry) {
      var nodes = [];
      for (var p = left; ; p = p.next) {
        nodes.push(p);
        if (p === right) break;
      }
      var leftIdx = this._indexOf(left);
      var points = elimPoints(len, round);
      var ev = {
        round: round,
        left: leftIdx,
        count: len,
        points: points,
        balls: nodes.map(function (n) { return n.color; }).join(""),
        removedIds: nodes.map(function (n) { return n.id; }),
        trackAfter: "",
        trackSnapshot: null
      };
      this.score = satAdd(this.score, points);
      entry.removed.push({
        left: leftIdx,
        balls: nodes.map(function (n) { return { color: n.color }; })
      });

      var after = right.next;
      this._eraseRun(left, len);
      ev.trackAfter = this.trackString();
      ev.trackSnapshot = this.trackNodes;
      events.push(ev);
      return after;
    }

    _settleFrom(probe, events, entry) {
      var round = 0;
      var first = true;
      while (probe) {
        round++;
        if (first && probe.color === BOMB) {
          // 炸弹首轮：右侧优先扩展到至多 3 球；不足时消到端点。
          var segLeft = probe;
          var segRight = probe;
          var len = 1;
          while (len < 3) {
            var grew = false;
            if (segRight.next) {
              segRight = segRight.next;
              len++;
              grew = true;
            }
            if (len >= 3) break;
            if (segLeft.prev) {
              segLeft = segLeft.prev;
              len++;
              grew = true;
            }
            if (!grew) break;
          }
          var nextBomb = this._eliminateRun(
            segLeft, segRight, len, round, events, entry);
          probe = nextBomb;
          first = false;
          continue;
        }
        first = false;

        var run = this._findColorRun(probe);
        if (run.len < 3) break;
        probe = this._eliminateRun(run.left, run.right, run.len, round,
                                   events, entry);
      }
    }

    _refreshStatus() {
      if (this.status === "quit") return;
      if (this.size === 0) {
        this.status = "win";
        return;
      }
      if (this.queueBalls.length === 0 ||
          (this.limit > 0 && this.moves >= this.limit)) {
        this.status = "lose";
        return;
      }
      this.status = "playing";
    }

    /* ---------------- 公开操作 ---------------- */

    shoot(pos, color) {
      if (this.status !== "playing")
        throw new ProtocolError("终局态不可发射");
      if (this.queueBalls.length === 0)
        throw new ProtocolError("待发射队列为空");
      if (!Number.isInteger(pos) || pos < 0 || pos > this.size)
        throw new ProtocolError("发射位置越界");

      var colorChar = null;
      if (typeof color === "number" && color > 0)
        colorChar = String.fromCharCode(color);
      else if (typeof color === "string" && color.length === 1)
        colorChar = color;
      else if (color !== undefined && color !== null && color !== 0 &&
               color !== "")
        throw new ProtocolError("仅变色球可指定颜色");

      var front = this.queueBalls[0];
      if (front.color === CHAMELEON) {
        if (!colorChar || this.colors.indexOf(colorChar) === -1)
          throw new ProtocolError("变色球必须指定 COLORS 内的颜色");
      } else if (colorChar !== null) {
        throw new ProtocolError("仅变色球可指定颜色");
      }

      var shotBall = this.queueBalls.shift();
      var ballColor = shotBall.color === CHAMELEON ? colorChar : shotBall.color;

      var entry = {
        op: "shoot",
        pos: pos,
        shotBall: shotBall,
        removed: [],
        scoreBefore: this.score,
        movesBefore: this.moves
      };

      var node = this._insertAt(pos, ballColor);
      var preSettle = this.trackNodes;
      var events = [];
      this._settleFrom(node, events, entry);

      this.moves++;
      this.undoStack.push(entry);
      this._refreshStatus();
      return {
        op: "shoot",
        events: events,
        preSettle: preSettle,
        insertedId: node.id,
        final: this.snapshot()
      };
    }

    swap() {
      // 与 shoot()/reverse() 对齐：终局后拒绝，且不改变任何状态。
      if (this.status !== "playing")
        throw new ProtocolError("终局态不可交换");
      if (this.queueBalls.length < 2)
        throw new ProtocolError("队列不足两球，无法交换");
      var tmp = this.queueBalls[0];
      this.queueBalls[0] = this.queueBalls[1];
      this.queueBalls[1] = tmp;
      this.undoStack.push({
        op: "swap",
        pos: 0,
        extra: 0,
        removed: [],
        scoreBefore: this.score,
        movesBefore: this.moves
      });
      this.moves++;
      this._refreshStatus();
      return { op: "swap", events: [], final: this.snapshot() };
    }

    reverse(i, j) {
      if (this.status !== "playing")
        throw new ProtocolError("终局态不可反转");
      if (!Number.isInteger(i) || !Number.isInteger(j) ||
          i < 0 || j < i || j >= this.size)
        throw new ProtocolError("反转区间越界");

      var entry = {
        op: "reverse",
        pos: i,
        extra: j,
        removed: [],
        scoreBefore: this.score,
        movesBefore: this.moves
      };
      this._reverseRange(i, j);
      var preSettle = this.trackNodes;

      // 与 C++ 一致：两处拼缝（i-1/i 与 j/j+1）各取内侧节点为 probe。
      // 第一次 settle 后仅用 alive 标记判断第二个 probe 是否幸存（不访问
      // 已删节点的链接）。
      var probeA = this.at(i);
      var probeB = this.at(j);
      var events = [];
      this._settleFrom(probeA, events, entry);
      if (probeB && probeB.alive)
        this._settleFrom(probeB, events, entry);

      this.undoStack.push(entry);
      this.moves++;
      this._refreshStatus();
      return {
        op: "reverse",
        events: events,
        preSettle: preSettle,
        final: this.snapshot()
      };
    }

    hint() {
      if (this.status !== "playing" || this.queueBalls.length === 0)
        return -1;
      var ball = this.queueBalls[0].color;
      if (ball === CHAMELEON) return -1;

      var colors = [];
      for (var p = this.head; p; p = p.next) colors.push(p.color);
      var n = colors.length;
      var bestPos = -1;
      var bestElim = 0;

      for (var pos = 0; pos <= n; pos++) {
        var elim = 0;
        if (ball === BOMB) {
          elim = Math.min(3, n + 1); // 端点不足一侧时继续向另一侧扩展
        } else {
          var sim = colors.slice();
          sim.splice(pos, 0, ball);
          var l = pos;
          var r = pos;
          while (l > 0 && sim[l - 1] === ball) l--;
          while (r + 1 < sim.length && sim[r + 1] === ball) r++;
          var len = r - l + 1;
          if (len >= 3) elim = len;
        }
        if (elim > bestElim) {
          bestElim = elim;
          bestPos = pos;
        }
      }
      return bestPos;
    }

    undo() {
      if (this.status === "quit" || this.undoStack.length === 0) return false;
      var entry = this.undoStack.pop();
      this.score = entry.scoreBefore;
      this.moves = entry.movesBefore;
      this.status = "playing";

      if (entry.op === "shoot") {
        for (var k = entry.removed.length - 1; k >= 0; k--) {
          var run = entry.removed[k];
          var anchor = run.left < this.size ? this.at(run.left) : null;
          this._insertBefore(anchor, run.balls);
        }
        if (entry.pos < this.size) this._eraseNode(this.at(entry.pos));
        this.queueBalls.unshift(entry.shotBall);
      } else if (entry.op === "swap") {
        if (this.queueBalls.length >= 2) {
          var tmp = this.queueBalls[0];
          this.queueBalls[0] = this.queueBalls[1];
          this.queueBalls[1] = tmp;
        }
      } else if (entry.op === "reverse") {
        for (var r = entry.removed.length - 1; r >= 0; r--) {
          var rr = entry.removed[r];
          var anchorR = rr.left < this.size ? this.at(rr.left) : null;
          this._insertBefore(anchorR, rr.balls);
        }
        this._reverseRange(entry.pos, entry.extra);
      }

      this._refreshStatus();
      return true;
    }

    quit() {
      this.status = "quit";
    }
  }

  return {
    Engine: Engine,
    ProtocolError: ProtocolError,
    BOMB: BOMB,
    CHAMELEON: CHAMELEON
  };
});