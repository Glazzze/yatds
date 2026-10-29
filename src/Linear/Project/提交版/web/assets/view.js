/*
 * 祖玛 · 网页视图层（阶段 5b）
 *
 * 只负责 DOM、布局、动画与可访问性，不改变游戏状态；状态变化一律由
 * app.js 调用 Controller 后，把快照交给这里渲染。轨道采用弧线布局：
 * 每个球用稳定 id 复用 DOM，插入/消除时只移动或移除对应节点，避免整轨
 * 重绘造成动画跳变。所有快照数据仍以 C++ / PROTOCOL 的语义为准。
 */
(function (root) {
  "use strict";

  var doc = root.document;
  var BOMB = "*";
  var CHAMELEON = "?";

  var PALETTE = {
    R: { color: "#E8756B", ink: "#4A1F1B", name: "红球" },
    G: { color: "#76C293", ink: "#1D4630", name: "绿球" },
    B: { color: "#68A5DC", ink: "#14354D", name: "蓝球" },
    Y: { color: "#F2C75C", ink: "#5C4210", name: "黄球" },
    M: { color: "#D190BE", ink: "#4B2340", name: "粉球" },
    C: { color: "#6CC8C6", ink: "#174845", name: "青球" },
    W: { color: "#F3EEE3", ink: "#6B6254", name: "白球" },
    K: { color: "#59646D", ink: "#FFFFFF", name: "黑球" }
  };

  /* 未使用助记字母时的后备色板（按 COLORS 声明顺序轮换）。 */
  var FALLBACK = [
    { color: "#E8756B", ink: "#4A1F1B" },
    { color: "#76C293", ink: "#1D4630" },
    { color: "#68A5DC", ink: "#14354D" },
    { color: "#F2C75C", ink: "#5C4210" },
    { color: "#D190BE", ink: "#4B2340" },
    { color: "#6CC8C6", ink: "#174845" },
    { color: "#F3EEE3", ink: "#6B6254" },
    { color: "#59646D", ink: "#FFFFFF" }
  ];

  function ballInfo(color, colors) {
    if (color === BOMB)
      return { color: "#59646D", ink: "#FBEBD3", label: "✦", name: "炸弹", special: "bomb" };
    if (color === CHAMELEON)
      return { color: "#CBE7D8", ink: "#4A5B53", label: "?", name: "变色球", special: "chameleon" };
    var known = PALETTE[color];
    if (known)
      return { color: known.color, ink: known.ink, label: color, name: known.name, special: "" };
    var idx = colors ? colors.indexOf(color) : -1;
    if (idx < 0) idx = 0;
    var fb = FALLBACK[idx % FALLBACK.length];
    return { color: fb.color, ink: fb.ink, label: color || "?", name: "彩球 " + (color || "?"), special: "" };
  }

  function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }

  function escapeHtml(s) {
    return String(s).replace(/[&<>"']/g, function (c) {
      return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c];
    });
  }

  var KIND_LABEL = {
    shoot: "发射",
    swap: "交换",
    reverse: "倒退",
    hint: "提示",
    undo: "撤销"
  };

  var IDS = [
    "app", "board", "level-button", "level-label", "score-value", "score-delta",
    "progress-text", "progress-bar", "progress-fill", "moves-text", "chain-text",
    "sound-button", "track-shell", "track-world", "track-svg", "track-path",
    "track-path-inner", "track-balls", "track-slots", "particle-layer",
    "position-ruler", "track-empty", "hint-badge", "range-bar", "range-text",
    "aim-line", "launcher", "launcher-socket", "queue-tray", "queue-count",
    "track-count", "queue-remaining", "limit-chip", "undo-button", "hint-button",
    "swap-button", "reverse-button", "preview-banner", "preview-text",
    "history-drawer", "history-list", "history-close", "drawer-scrim",
    "level-grid", "custom-text", "custom-error", "custom-load",
    "random-preview", "random-again", "random-copy", "random-load",
    "pause-resume", "pause-restart", "pause-sound",
    "result-emblem", "result-title", "result-sub", "result-score",
    "result-moves", "result-chain", "result-cleared", "result-undo",
    "result-restart", "toasts", "live", "color-popover", "color-swatches",
    "preview-return", "ghost-ball"
  ];

  class View {
    constructor(handlers) {
      this.handlers = handlers || {};
      this.els = {};
      IDS.forEach((id) => {
        this.els[id.replace(/-([a-z])/g, function (_, c) { return c.toUpperCase(); })] =
          doc.getElementById(id);
      });
      this.els.trackZone = this.els.trackShell.parentNode;
      this.els.appEl = this.els.app;

      this.ballEls = new Map();
      this.snapshot = null;
      this.currentTrack = [];
      this.currentQueue = [];
      this.colors = "";
      this.frontColor = null;
      this.status = "playing";
      this.selected = 0;
      this.hoverSlot = null;
      this.layout = null;
      this.busy = false;
      this.skipFlag = false;
      this.preview = false;
      this.reduced = !!(root.matchMedia &&
        root.matchMedia("(prefers-reduced-motion: reduce)").matches);

      this._sleepResolve = null;
      this._hintTimer = 0;
      this._resizeTimer = 0;
      this._lastFocus = null;
      this._modalStack = [];

      this._buildRulerInner();
      this._bind();
    }

    _call(name) {
      var fn = this.handlers[name];
      if (typeof fn !== "function") return undefined;
      return fn.apply(null, Array.prototype.slice.call(arguments, 1));
    }

    _buildRulerInner() {
      var inner = doc.createElement("div");
      inner.className = "ruler-inner";
      this.els.positionRuler.appendChild(inner);
      this.rulerInner = inner;
    }

    _bind() {
      var self = this;

      this.els.trackSlots.addEventListener("click", function (e) {
        var slot = e.target.closest(".slot");
        if (!slot) return;
        self._call("onFire", Number(slot.dataset.pos));
      });

      this.els.trackSlots.addEventListener("mouseover", function (e) {
        var slot = e.target.closest(".slot");
        if (!slot || slot === self.hoverSlot) return;
        self.hoverSlot = slot;
        self._call("onSelect", Number(slot.dataset.pos), false);
      });

      this.els.trackSlots.addEventListener("mouseleave", function () {
        self.hoverSlot = null;
      });

      this.els.launcher.addEventListener("click", function () {
        self._call("onLauncherClick");
      });

      this.els.trackBalls.addEventListener("click", function (e) {
        var ball = e.target.closest(".track-ball");
        if (!ball) return;
        var idx = self.currentTrack.findIndex(function (n) {
          return String(n.id) === ball.dataset.id;
        });
        if (idx >= 0) self._call("onBallClick", idx);
      });

      this.els.launcher.addEventListener("keydown", function (e) {
        if (e.key === "Enter" || e.key === " ") {
          e.preventDefault();
          self._call("onLauncherClick");
        }
      });
      this.els.launcher.tabIndex = 0;
      this.els.launcher.setAttribute("role", "button");
      this.els.launcher.setAttribute("aria-label", "发射器，查看队首球");

      this.els.historyList.addEventListener("click", function (e) {
        var btn = e.target.closest(".history-btn");
        if (!btn) return;
        self._call("onHistoryPreview", Number(btn.dataset.index));
      });

      this.els.levelGrid.addEventListener("click", function (e) {
        var card = e.target.closest(".level-card");
        if (!card) return;
        self._call("onLevelPick", card.dataset.id);
      });

      this.els.colorSwatches.addEventListener("click", function (e) {
        var swatch = e.target.closest(".swatch");
        if (!swatch) return;
        self._call("onColorPick", swatch.dataset.color);
      });

      this.els.historyClose.addEventListener("click", function () {
        self.closeDrawer();
      });
      this.els.drawerScrim.addEventListener("click", function () {
        self.closeDrawer();
      });
      this.els.previewReturn.addEventListener("click", function () {
        self._call("onPreviewReturn");
      });

      doc.addEventListener("click", function (e) {
        var close = e.target.closest("[data-close]");
        if (close) self.closeModal(close.dataset.close);
        var open = e.target.closest("[data-open]");
        if (open) self.openModal(open.dataset.open);
      });

      doc.querySelectorAll(".modal-layer").forEach(function (layer) {
        layer.addEventListener("keydown", function (e) {
          if (e.key !== "Tab") return;
          var items = self._focusables(layer);
          if (!items.length) return;
          var first = items[0], last = items[items.length - 1];
          if (e.shiftKey && doc.activeElement === first) {
            e.preventDefault();
            last.focus();
          } else if (!e.shiftKey && doc.activeElement === last) {
            e.preventDefault();
            first.focus();
          }
        });
      });

      root.addEventListener("resize", function () {
        clearTimeout(self._resizeTimer);
        self._resizeTimer = setTimeout(function () { self.relayout(); }, 120);
      });

      this.els.trackShell.addEventListener("scroll", function () {
        if (self.rulerInner && self.layout) {
          self.rulerInner.style.transform = "translateX(" +
            (-self.els.trackShell.scrollLeft) + "px)";
        }
        self._updateAim();
      }, { passive: true });
    }

    _focusables(layer) {
      return Array.prototype.slice.call(layer.querySelectorAll(
        'button:not(:disabled), [href], input:not(:disabled), textarea:not(:disabled), [tabindex]:not([tabindex="-1"])'
      )).filter(function (el) { return el.offsetParent !== null; });
    }

    /* ---------------- 轨道布局 ---------------- */

    static layoutFor(n, shellWidth) {
      var W = Math.max(286, shellWidth || 760, n * 62 + 106);
      var pad = 53;
      var top = 86;
      var lift = 158;
      return {
        W: W,
        n: n,
        pad: pad,
        top: top,
        lift: lift,
        x: function (t) { return pad + t * (W - pad * 2); },
        y: function (t) { return top - lift * t * (1 - t); },
        slotT: function (p) { return n > 0 ? p / n : 0.5; },
        ballT: function (i) { return n > 0 ? (i + 0.5) / n : 0.5; },
        cx: W / 2,
        cy: top - lift / 2
      };
    }

    _makeLayout(n) {
      var shellW = this.els.trackShell.clientWidth || 760;
      this.layout = View.layoutFor(n, shellW);
      return this.layout;
    }

    _trackPath() {
      var L = this.layout, pad = L.pad;
      return "M " + pad + " " + L.top +
        " Q " + L.cx + " " + L.cy + " " + (L.W - pad) + " " + L.top;
    }

    /* ---------------- 基础渲染 ---------------- */

    _faceEl(color) {
      var info = ballInfo(color, this.colors);
      var face = doc.createElement("div");
      face.className = "ball-face" +
        (info.special ? " is-" + info.special : "") +
        (color === "W" ? " is-white" : "");
      face.style.setProperty("--ball-color", info.color);
      face.style.setProperty("--ball-ink", info.ink);
      face.title = info.name;
      face.dataset.color = color;
      var letter = doc.createElement("span");
      letter.className = "ball-letter";
      letter.textContent = info.label;
      face.appendChild(letter);
      return face;
    }

    _applyFace(face, color) {
      var info = ballInfo(color, this.colors);
      face.className = "ball-face" +
        (info.special ? " is-" + info.special : "") +
        (color === "W" ? " is-white" : "");
      face.style.setProperty("--ball-color", info.color);
      face.style.setProperty("--ball-ink", info.ink);
      face.title = info.name;
      var letter = face.querySelector(".ball-letter");
      if (letter) letter.textContent = info.label;
    }

    renderTrack(track, opts) {
      opts = opts || {};
      this.currentTrack = track.map(function (n) { return { id: n.id, color: n.color }; });
      if (opts.colors) this.colors = opts.colors;
      var n = this.currentTrack.length;
      var L = this._makeLayout(n);

      this.els.trackWorld.style.width = L.W + "px";
      var H = this.els.trackWorld.clientHeight || 146;
      this.els.trackSvg.setAttribute("viewBox", "0 0 " + L.W + " " + H);
      var d = this._trackPath();
      this.els.trackPath.setAttribute("d", d);
      this.els.trackPathInner.setAttribute("d", d);

      var seen = new Set();
      this.currentTrack.forEach((node, i) => {
        seen.add(node.id);
        var x = L.x(L.ballT(i));
        var y = L.y(L.ballT(i));
        var el = this.ballEls.get(node.id);
        if (!el) {
          el = doc.createElement("div");
          el.className = "track-ball";
          el.dataset.id = String(node.id);
          el.style.left = x + "px";
          el.style.top = y + "px";
          el.appendChild(this._faceEl(node.color));
          this.els.trackBalls.appendChild(el);
          this.ballEls.set(node.id, el);
          el.classList.add("is-new");
          root.setTimeout(() => el.classList.remove("is-new"), this.reduced ? 20 : 280);
        } else {
          el.style.left = x + "px";
          el.style.top = y + "px";
          var face = el.firstElementChild;
          if (face && face.dataset.color !== node.color) {
            this._applyFace(face, node.color);
          }
          face.dataset.color = node.color;
        }
        el.setAttribute("aria-label", ballInfo(node.color, this.colors).name +
          "，第 " + (i + 1) + " 颗");
        if (opts.newId && node.id === opts.newId) el.classList.add("is-new");
      });

      this.ballEls.forEach((el, id) => {
        if (!seen.has(id)) {
          el.remove();
          this.ballEls.delete(id);
        }
      });

      this._renderSlots(n, L);
      this.els.trackEmpty.hidden = n !== 0;
      this.els.trackCount.textContent = "轨道 " + n;

      if (this.selected < 0) this.selected = 0;
      if (this.selected > n) this.selected = n;
      this.setSelected(this.selected);
    }

    _renderSlots(n, L) {
      var frag = doc.createDocumentFragment();
      for (var p = 0; p <= n; p++) {
        var slot = doc.createElement("button");
        slot.type = "button";
        slot.className = "slot";
        slot.dataset.pos = String(p);
        slot.tabIndex = -1;
        var x = L.x(L.slotT(p));
        var y = L.y(L.slotT(p));
        slot.style.left = x + "px";
        slot.style.top = y + "px";
        slot.setAttribute("aria-label", n === 0
          ? "在轨道中发射"
          : "在第 " + p + " 个空隙插入，共 " + (n + 1) + " 个位置");
        frag.appendChild(slot);
      }
      this.els.trackSlots.replaceChildren(frag);
    }

    setSelected(pos, opts) {
      opts = opts || {};
      var n = this.currentTrack.length;
      pos = clamp(Number(pos) || 0, 0, n);
      this.selected = pos;
      this.els.trackSlots.querySelectorAll(".slot").forEach(function (slot) {
        slot.classList.toggle("is-selected", Number(slot.dataset.pos) === pos);
      });
      this._renderRuler();
      if (opts.reveal) this._revealSlot(pos);
      this._updateGhost();
      this._updateAim();
    }

    _revealSlot(pos) {
      var slot = this.els.trackSlots.querySelector('.slot[data-pos="' + pos + '"]');
      if (!slot) return;
      var shell = this.els.trackShell;
      var left = parseFloat(slot.style.left) || 0;
      var margin = 70;
      if (left < shell.scrollLeft + margin) {
        shell.scrollLeft = Math.max(0, left - margin);
      } else if (left > shell.scrollLeft + shell.clientWidth - margin) {
        shell.scrollLeft = left - shell.clientWidth + margin;
      }
    }

    _renderRuler() {
      if (!this.layout) return;
      var L = this.layout, n = this.currentTrack.length;
      var inner = this.rulerInner;
      inner.style.width = L.W + "px";
      inner.style.height = "100%";
      inner.style.position = "relative";
      inner.style.transform = "translateX(" + (-this.els.trackShell.scrollLeft) + "px)";
      var step = n <= 18 ? 1 : 5;
      var html = "";
      for (var p = 0; p <= n; p++) {
        if (p !== 0 && p !== n && p % step !== 0 && p !== this.selected) continue;
        var x = L.x(L.slotT(p));
        html += '<span class="ruler-tick' + (p === this.selected ? " is-selected" : "") +
          '" style="left:' + x + 'px">' + p + "</span>";
      }
      inner.innerHTML = html;
    }

    _updateGhost() {
      var ghost = this.els.ghostBall;
      if (this.preview || this.busy || this.status !== "playing" ||
          !this.frontColor || !this.layout) {
        ghost.hidden = true;
        return;
      }
      var L = this.layout;
      var t = L.slotT(this.selected);
      var x = L.x(t);
      var y = L.y(t);
      ghost.style.left = x + "px";
      ghost.style.top = y + "px";
      ghost.replaceChildren(this._faceEl(this.frontColor));
      ghost.hidden = false;
    }

    _updateAim() {
      var line = this.els.aimLine;
      if (this.preview || this.busy || this.status !== "playing" ||
          !this.frontColor || !this.selected) {
        if (!this.frontColor || this.status !== "playing") {
          line.hidden = true;
          return;
        }
      }
      var slot = this.els.trackSlots.querySelector('.slot[data-pos="' + this.selected + '"]');
      if (!slot || !this.frontColor || this.preview || this.busy) {
        line.hidden = true;
        return;
      }
      var boardRect = this.els.board.getBoundingClientRect();
      var lRect = this.els.launcher.getBoundingClientRect();
      var sRect = slot.getBoundingClientRect();
      var x1 = lRect.left + lRect.width / 2 - boardRect.left;
      var y1 = lRect.top + lRect.height / 2 - boardRect.top;
      var x2 = sRect.left + sRect.width / 2 - boardRect.left;
      var y2 = sRect.top + sRect.height / 2 - boardRect.top;
      var dx = x2 - x1, dy = y2 - y1;
      var len = Math.sqrt(dx * dx + dy * dy);
      if (len < 20) {
        line.hidden = true;
        return;
      }
      line.style.left = x1 + "px";
      line.style.top = y1 + "px";
      line.style.width = len + "px";
      line.style.transform = "rotate(" + Math.atan2(dy, dx) + "rad)";
      line.hidden = false;
    }

    renderQueue(queue, opts) {
      opts = opts || {};
      this.currentQueue = queue.map(function (b) { return { id: b.id, color: b.color }; });
      this.frontColor = queue.length ? queue[0].color : null;

      var socket = this.els.launcherSocket;
      socket.replaceChildren();
      socket.classList.toggle("is-empty", queue.length === 0);
      if (queue.length) socket.appendChild(this._faceEl(queue[0].color));
      else socket.removeAttribute("title");

      var tray = this.els.queueTray;
      tray.replaceChildren();
      var visible = queue.slice(1, 7);
      visible.forEach((ball, idx) => {
        var wrap = doc.createElement("div");
        wrap.className = "queue-ball" + (opts.swap && idx === 0 ? " is-swapping" : "");
        wrap.appendChild(this._faceEl(ball.color));
        wrap.title = ballInfo(ball.color, this.colors).name;
        tray.appendChild(wrap);
      });
      if (queue.length > 7) {
        var more = doc.createElement("span");
        more.className = "queue-more";
        more.textContent = "+" + (queue.length - 7);
        tray.appendChild(more);
      }
      if (queue.length === 0) {
        var empty = doc.createElement("span");
        empty.className = "queue-empty";
        empty.textContent = "队列已空";
        tray.appendChild(empty);
      }
      this.els.queueCount.textContent = queue.length ? String(queue.length) : "0";
      this.els.queueRemaining.textContent = "队列 " + queue.length;

      if (opts.swap) {
        this.els.launcher.classList.add("is-swapping");
        root.setTimeout(() => this.els.launcher.classList.remove("is-swapping"), 300);
      }
      this._updateGhost();
      this._updateAim();
    }

    renderHud(snapshot, opts) {
      opts = opts || {};
      var total = snapshot.initialTotal || 0;
      var remain = snapshot.track.length + snapshot.queue.length;
      var cleared = Math.max(0, total - remain);
      var pct = total > 0 ? clamp((1 - remain / total) * 100, 0, 100) : 100;

      this.els.scoreValue.textContent = String(snapshot.score);
      this.els.progressText.textContent = "已消除 " + cleared + " / " + total;
      this.els.progressFill.style.width = pct + "%";
      this.els.progressBar.setAttribute("aria-valuenow", String(Math.round(pct)));

      var moves = "步数 " + snapshot.moves;
      if (snapshot.limit > 0) moves += " / " + snapshot.limit;
      this.els.movesText.textContent = moves;
      this.els.chainText.textContent = opts.chainPeak > 1
        ? "最高连锁 ×" + opts.chainPeak : "";

      this.els.limitChip.hidden = !(snapshot.limit > 0);
      if (snapshot.limit > 0) {
        this.els.limitChip.textContent = "上限 " + snapshot.limit;
        this.els.limitChip.classList.toggle("is-warn",
          snapshot.limit - snapshot.moves <= 1);
      }

      if (opts.delta) this.showScoreDelta(opts.delta);
      this.status = snapshot.status;
      this.renderControls(snapshot);
    }

    showScoreDelta(delta) {
      var el = this.els.scoreDelta;
      el.textContent = (delta > 0 ? "+" : "") + delta;
      el.classList.toggle("is-negative", delta < 0);
      el.classList.remove("is-live");
      void el.offsetWidth;
      el.classList.add("is-live");
      root.setTimeout(() => el.classList.remove("is-live"), 800);
    }

    _setScore(score, points) {
      this.els.scoreValue.textContent = String(score);
      if (points) this.showScoreDelta(points);
    }

    renderControls(snapshot) {
      var disabled = this.busy || this.preview;
      var playing = snapshot.status === "playing";
      this.els.undoButton.disabled = disabled || !snapshot.undoAvailable;
      this.els.hintButton.disabled = disabled || !playing || snapshot.queue.length === 0;
      this.els.swapButton.disabled = disabled || !playing || snapshot.queue.length < 2;
      this.els.reverseButton.disabled = disabled || !playing || snapshot.track.length < 2;
      this.els.hintButton.classList.toggle("is-active", this._hintActive === true);
      this.els.reverseButton.classList.toggle("is-active", this._reverseActive === true);
    }

    render(snapshot, opts) {
      opts = opts || {};
      this.snapshot = snapshot;
      this.colors = snapshot.colors || this.colors;
      this.status = snapshot.status;
      this.preview = !!opts.preview;
      this.els.board.classList.toggle("is-preview", this.preview);
      this.renderTrack(snapshot.track, { colors: this.colors });
      this.renderQueue(snapshot.queue, opts);
      this.renderHud(snapshot, opts);
      this.setSelected(this.selected);
    }

    relayout() {
      if (!this.snapshot) return;
      this.renderTrack(this.snapshot.track, { colors: this.colors });
      this._updateAim();
    }

    /* ---------------- 动画 ---------------- */

    _sleep(ms) {
      if (this.reduced || this.skipFlag) return Promise.resolve();
      var self = this;
      return new Promise(function (resolve) {
        var done = false;
        var finish = function () {
          if (done) return;
          done = true;
          clearTimeout(timer);
          if (self._sleepResolve === finish) self._sleepResolve = null;
          resolve();
        };
        var timer = root.setTimeout(finish, ms);
        self._sleepResolve = finish;
      });
    }

    skip() {
      if (!this.busy) return;
      this.skipFlag = true;
      if (this._sleepResolve) this._sleepResolve();
    }

    _flashRemoval(ids) {
      var layer = this.els.particleLayer;
      if (layer.childElementCount > 100) return;
      ids.forEach((id) => {
        var el = this.ballEls.get(id);
        if (!el) return;
        var face = el.firstElementChild;
        var color = face ? face.style.getPropertyValue("--ball-color") : "#A9D8EA";
        var x = parseFloat(el.style.left) || 0;
        var y = parseFloat(el.style.top) || 0;
        el.classList.add("is-clearing");
        for (var k = 0; k < 5; k++) {
          var angle = Math.random() * Math.PI * 2;
          var dist = 15 + Math.random() * 24;
          var p = doc.createElement("span");
          p.className = "particle";
          p.style.left = x + "px";
          p.style.top = y + "px";
          p.style.setProperty("--pcolor", color || "#A9D8EA");
          p.style.setProperty("--dx", Math.cos(angle) * dist + "px");
          p.style.setProperty("--dy", (Math.sin(angle) * dist - 8) + "px");
          p.style.animationDelay = (k * 12) + "ms";
          layer.appendChild(p);
          root.setTimeout(() => p.remove(), 620);
        }
      });
    }

    _restoreBalls() {
      this.ballEls.forEach((el) => {
        el.classList.remove("is-new");
        el.classList.add("is-restoring");
        root.setTimeout(() => el.classList.remove("is-restoring"), 300);
      });
    }

    async playTurn(turn, controller) {
      this.busy = true;
      this.skipFlag = false;
      this.els.board.classList.add("is-busy");
      this.renderControls(controller.snapshot());
      var finalSnap = controller.snapshot();
      var before = turn.before;
      var events = turn.events || [];

      try {
        if (turn.kind === "shoot" || turn.kind === "reverse") {
          if (turn.preSettle) {
            this.renderTrack(turn.preSettle, {
              newId: turn.insertedId,
              colors: finalSnap.colors
            });
            this.renderQueue(finalSnap.queue);
            if (turn.kind === "shoot") this._call("onInsert", turn);
            else this._call("onReverse", turn);
            await this._sleep(turn.kind === "shoot" ? 190 : 250);
          }

          var score = before.score;
          for (var i = 0; i < events.length; i++) {
            if (this.skipFlag) break;
            var ev = events[i];
            this._flashRemoval(ev.removedIds || []);
            score += ev.points;
            this._setScore(score, ev.points);
            this._call("onEliminate", ev, turn);
            await this._sleep(230);
            this.renderTrack(ev.trackSnapshot || [], { colors: finalSnap.colors });
            await this._sleep(90);
          }

          this.render(finalSnap, {
            delta: finalSnap.score - before.score,
            chainPeak: controller.chainPeak
          });
        } else if (turn.kind === "swap") {
          this.render(finalSnap, { chainPeak: controller.chainPeak });
          this.renderQueue(finalSnap.queue, { swap: true });
          this._call("onSwap", turn);
          await this._sleep(220);
        } else if (turn.kind === "undo") {
          this.render(finalSnap, { chainPeak: controller.chainPeak });
          this._restoreBalls();
          this._call("onUndo", turn);
          await this._sleep(230);
        } else if (turn.kind === "hint") {
          this.render(finalSnap, { chainPeak: controller.chainPeak });
          if (turn.hintPos >= 0) {
            this.showHint(turn.hintPos);
            this._call("onHint", turn);
            await this._sleep(1100);
            this.hideHint();
          }
        } else {
          this.render(finalSnap, { chainPeak: controller.chainPeak });
        }
      } finally {
        this.busy = false;
        this.skipFlag = false;
        this.els.board.classList.remove("is-busy");
        this.renderControls(controller.snapshot());
        this._updateGhost();
        this._updateAim();
      }
    }

    /* ---------------- 提示 ---------------- */

    showHint(pos) {
      this.hideHint();
      var slot = this.els.trackSlots.querySelector('.slot[data-pos="' + pos + '"]');
      if (!slot) return;
      slot.classList.add("is-hint");
      this._hintActive = true;
      this.renderControls(this.snapshot || { status: "playing", undoAvailable: false, queue: [], track: [] });
      var zoneRect = this.els.trackZone.getBoundingClientRect();
      var sRect = slot.getBoundingClientRect();
      var badge = this.els.hintBadge;
      badge.style.left = (sRect.left - zoneRect.left + sRect.width / 2) + "px";
      badge.style.top = (sRect.top - zoneRect.top - 6) + "px";
      badge.hidden = false;
      badge.style.animation = "none";
      void badge.offsetWidth;
      badge.style.animation = "";
      this._hintTimer = root.setTimeout(() => this.hideHint(), 1700);
    }

    hideHint() {
      clearTimeout(this._hintTimer);
      this._hintTimer = 0;
      this._hintActive = false;
      this.els.hintBadge.hidden = true;
      this.els.trackSlots.querySelectorAll(".slot.is-hint").forEach(function (el) {
        el.classList.remove("is-hint");
      });
      if (this.snapshot) this.renderControls(this.snapshot);
    }

    /* ---------------- 历史 / 关卡 / 浮层 ---------------- */

    renderHistory(turns, activeIndex) {
      var list = this.els.historyList;
      if (!turns || turns.length === 0) {
        list.innerHTML = '<li class="history-empty">还没有操作，点击轨道空隙发射第一颗球。</li>';
        return;
      }
      var last = turns.length - 1;
      var active = typeof activeIndex === "number" ? activeIndex : last;
      var html = "";
      turns.forEach(function (turn, index) {
        var kind = KIND_LABEL[turn.kind] || turn.kind;
        var delta = turn.scoreDelta || 0;
        var deltaText = delta > 0 ? "+" + delta : String(delta);
        var meta = kind + " · 第 " + turn.seq + " 步";
        if (turn.eliminated) meta += " · 消除 " + turn.eliminated + " 球";
        html += '<li class="history-item"><button class="history-btn' +
          (index === active ? " is-current" : "") +
          '" type="button" data-index="' + index + '">' +
          '<span class="history-seq">' + turn.seq + "</span>" +
          '<span class="history-main"><span class="history-label">' +
          escapeHtml(turn.label) + '</span><span class="history-meta">' +
          escapeHtml(meta) + "</span></span>" +
          '<span class="history-delta' + (delta === 0 ? " is-zero" : "") + '">' +
          deltaText + "</span></button></li>";
      });
      list.innerHTML = html;
      if (active === last) list.scrollTop = list.scrollHeight;
    }

    setHistoryActive(index) {
      this.els.historyList.querySelectorAll(".history-btn").forEach(function (btn) {
        btn.classList.toggle("is-current", Number(btn.dataset.index) === index);
      });
    }

    openDrawer() {
      this.els.historyDrawer.classList.add("is-open");
      this.els.historyDrawer.setAttribute("aria-hidden", "false");
      this.els.drawerScrim.hidden = false;
      this.els.historyClose.focus();
    }

    closeDrawer() {
      this.els.historyDrawer.classList.remove("is-open");
      this.els.historyDrawer.setAttribute("aria-hidden", "true");
      this.els.drawerScrim.hidden = true;
      this._call("onDrawerClose");
    }

    populateLevels(samples, activeId) {
      var html = samples.map(function (s) {
        var colors = /^COLORS\s+(\S+)/m.exec(s.input);
        var track = /^TRACK\s+(\S*)/m.exec(s.input);
        var queue = /^QUEUE\s+(\S*)/m.exec(s.input);
        var meta = (colors ? colors[1] : "?") + " 色 · 轨 " +
          (track ? track[1].length : "?") + " · 队 " +
          (queue ? queue[1].length : "?");
        return '<button class="level-card' + (s.id === activeId ? " is-active" : "") +
          '" type="button" data-id="' + escapeHtml(s.id) + '">' +
          '<span class="level-card-num">CASE ' + escapeHtml(s.num) + "</span>" +
          '<span class="level-card-title">' + escapeHtml(s.title) + "</span>" +
          '<span class="level-card-meta">' + escapeHtml(meta) + "</span></button>";
      }).join("");
      this.els.levelGrid.innerHTML = html;
    }

    markLevel(id) {
      this.els.levelGrid.querySelectorAll(".level-card").forEach(function (card) {
        card.classList.toggle("is-active", card.dataset.id === id);
      });
    }

    setLevelLabel(text) {
      this.els.levelLabel.textContent = text;
    }

    openModal(id) {
      var layer = doc.getElementById(id);
      if (!layer) return;
      if (layer.hidden) this._lastFocus = doc.activeElement;
      layer.hidden = false;
      if (this._modalStack.indexOf(id) < 0) this._modalStack.push(id);
      var items = this._focusables(layer);
      if (items.length) items[0].focus();
    }

    closeModal(id) {
      var layer = doc.getElementById(id);
      if (!layer) return;
      // 结算页关闭（撤销/重开/选关/Esc）时一并收掉彩带，避免它继续
      // 盖在随后打开的帮助或关卡弹窗上方（彩带 z-index 高于弹窗）。
      if (id === "result-modal") this.clearCelebration();
      layer.hidden = true;
      this._modalStack = this._modalStack.filter(function (x) { return x !== id; });
      if (this._modalStack.length === 0 && this._lastFocus && this._lastFocus.focus) {
        this._lastFocus.focus();
        this._lastFocus = null;
      }
    }

    closeTopModal() {
      if (this._modalStack.length === 0) return false;
      this.closeModal(this._modalStack[this._modalStack.length - 1]);
      return true;
    }

    get topModal() {
      return this._modalStack.length
        ? this._modalStack[this._modalStack.length - 1] : null;
    }

    get modalOpen() { return this._modalStack.length > 0; }

    showColorPicker(colors) {
      var html = colors.split("").map(function (c, i) {
        var info = ballInfo(c, colors);
        return '<button class="swatch" type="button" data-color="' + escapeHtml(c) +
          '" style="--swatch-color:' + info.color + ";--swatch-ink:" + info.ink +
          '" aria-label="选择 ' + escapeHtml(c) + ' 色">' + escapeHtml(c) +
          '<span class="swatch-key">' + (i + 1) + "</span></button>";
      }).join("");
      this.els.colorSwatches.innerHTML = html;
      var pop = this.els.colorPopover;
      pop.hidden = false;
      var rect = this.els.launcher.getBoundingClientRect();
      var w = pop.offsetWidth || 258;
      var h = pop.offsetHeight || 150;
      var left = clamp(rect.left + rect.width / 2 - w / 2, 12, root.innerWidth - w - 12);
      var top = rect.top - h - 12;
      if (top < 10) top = rect.bottom + 12;
      pop.style.left = left + "px";
      pop.style.top = top + "px";
      var first = pop.querySelector(".swatch");
      if (first) first.focus();
    }

    hideColorPicker() {
      this.els.colorPopover.hidden = true;
    }

    get colorPickerOpen() { return !this.els.colorPopover.hidden; }

    setSoundState(on) {
      this.els.soundButton.classList.toggle("is-on", on);
      var use = this.els.soundButton.querySelector("use");
      if (use) use.setAttribute("href", on ? "#i-sound" : "#i-mute");
      this.els.soundButton.setAttribute("aria-label", on ? "关闭音效" : "开启音效");
      this.els.pauseSound.textContent = "音效：" + (on ? "开" : "关");
    }

    setCustomError(message) {
      var el = this.els.customError;
      if (message) {
        el.textContent = message;
        el.hidden = false;
      } else {
        el.hidden = true;
      }
    }

    setRandomPreview(text) {
      this.els.randomPreview.textContent = text;
    }

    showResult(data) {
      var win = data.status === "win";
      var emblem = this.els.resultEmblem;
      emblem.classList.toggle("is-lose", !win);
      this.els.resultTitle.textContent = win ? "轨道清空！" : "差一点就清空了";
      this.els.resultSub.textContent = data.sub || (win
        ? "彩球全部消除，漂亮的一局。"
        : "队列或步数已耗尽，试试撤销最后一步。");
      this.els.resultScore.textContent = String(data.score);
      this.els.resultMoves.textContent = String(data.moves);
      this.els.resultChain.textContent = "×" + Math.max(1, data.chainPeak || 1);
      this.els.resultCleared.textContent = String(data.cleared || 0);
      this.els.resultUndo.disabled = !data.undoAvailable;
      this.openModal("result-modal");
      if (win) this.celebrate();
    }

    celebrate() {
      if (this.reduced) return;
      this.clearCelebration();
      var layer = doc.createElement("div");
      layer.className = "confetti-layer";
      layer.setAttribute("aria-hidden", "true");
      var palette = ["#E8756B", "#76C293", "#68A5DC", "#F2C75C", "#D190BE", "#6CC8C6", "#F3C27E"];
      for (var i = 0; i < 44; i++) {
        var piece = doc.createElement("span");
        piece.className = "confetti-piece";
        piece.style.left = (Math.random() * 100) + "%";
        piece.style.background = palette[i % palette.length];
        piece.style.setProperty("--drift", (Math.random() * 160 - 80) + "px");
        piece.style.setProperty("--spin", (Math.random() * 720 + 180) + "deg");
        piece.style.setProperty("--fall-duration", (1000 + Math.random() * 600) + "ms");
        piece.style.setProperty("--fall-delay", (Math.random() * 260) + "ms");
        layer.appendChild(piece);
      }
      doc.body.appendChild(layer);
      root.setTimeout(function () { layer.remove(); }, 2300);
    }

    clearCelebration() {
      var old = doc.querySelector(".confetti-layer");
      if (old) old.remove();
    }

    showPreview(snapshot, label) {
      this.preview = true;
      this.previewLabel = label || "历史局面";
      this.els.previewText.textContent = this.previewLabel;
      this.els.previewBanner.hidden = false;
      this.render(snapshot, { preview: true });
    }

    clearPreview(snapshot) {
      this.preview = false;
      this.els.previewBanner.hidden = true;
      this.render(snapshot);
    }

    showReverseRange(a, b) {
      this.ballEls.forEach((el) => el.classList.remove("is-reverse"));
      if (a == null) return;
      var start = a, end = b == null ? a : b;
      if (end < start) { var t = start; start = end; end = t; }
      for (var i = start; i <= end; i++) {
        var node = this.currentTrack[i];
        if (!node) continue;
        var el = this.ballEls.get(node.id);
        if (el) el.classList.add("is-reverse");
      }
    }

    clearReverseRange() {
      this.ballEls.forEach((el) => el.classList.remove("is-reverse"));
      this.els.board.classList.remove("is-reverse");
      this.setReverseActive(false);
      this.showRangeBar(false);
    }

    setReverseMode(on) {
      this.els.board.classList.toggle("is-reverse", !!on);
      this.setReverseActive(!!on);
    }

    toast(message, type) {
      var stack = this.els.toasts;
      while (stack.childElementCount >= 3) stack.firstElementChild.remove();
      var t = doc.createElement("div");
      t.className = "toast" + (type ? " is-" + type : "");
      t.textContent = message;
      stack.appendChild(t);
      root.setTimeout(function () {
        t.classList.add("is-out");
        root.setTimeout(function () { t.remove(); }, 220);
      }, 2100);
    }

    announce(message) {
      this.els.live.textContent = "";
      root.setTimeout(() => { this.els.live.textContent = message; }, 20);
    }

    setReverseActive(on) {
      this._reverseActive = !!on;
      if (this.snapshot) this.renderControls(this.snapshot);
    }

    setRangeLabel(text) {
      this.els.rangeText.textContent = text;
    }

    showRangeBar(on) {
      this.els.rangeBar.hidden = !on;
    }

    focusBoard() {
      this.els.board.focus({ preventScroll: true });
    }
  }

  root.Zuma = root.Zuma || {};
  root.Zuma.View = View;
  root.Zuma.ballInfo = ballInfo;
})(typeof globalThis !== "undefined" ? globalThis : this);
