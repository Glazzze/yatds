/*
 * 祖玛 · 网页应用层（阶段 5b）
 *
 * 状态机：
 *   aim → animating → aim/examine；preview / reverse / paused / result 为
 *   四个互斥的辅助状态。所有游戏规则与快照来自 Controller/Engine，本文件
 *   只负责输入、确认流程、弹窗、音效开关与 localStorage。
 */
(function (root) {
  "use strict";

  var Zuma = root.Zuma;
  var doc = root.document;
  var samples = Zuma.samples || [];
  var DEFAULT_SAMPLE = "case03";
  var STORE_SOUND = "zuma.sound";
  var STORE_BEST = "zuma.best";

  var app = {
    view: null,
    sound: null,
    controller: null,
    baseScenario: null,
    baseMeta: null,
    levelId: null,
    state: "aim",
    selection: 0,
    previewIndex: null,
    chainPeak: 0,
    reverse: { a: null, b: null },
    pendingLoad: null,
    random: null,
    randomText: "",
    resultTimer: 0,
    loadToken: 0
  };

  function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }

  function sampleById(id) {
    for (var i = 0; i < samples.length; i++)
      if (samples[i].id === id) return samples[i];
    return null;
  }

  function parseSample(sample) {
    var parsed = Zuma.parseInput(sample.input);
    if (!parsed.ok) return { error: parsed.error };
    return {
      scenario: {
        colors: parsed.colors,
        track: parsed.track,
        queue: parsed.queue,
        limit: parsed.limit || 0
      }
    };
  }

  function readStore(key, fallback) {
    try {
      var v = root.localStorage.getItem(key);
      return v == null ? fallback : v;
    } catch (e) {
      return fallback;
    }
  }

  function writeStore(key, value) {
    try { root.localStorage.setItem(key, value); } catch (e) { /* ignore */ }
  }

  function soundPref() {
    return readStore(STORE_SOUND, "0") === "1";
  }

  function readBest() {
    try {
      var raw = readStore(STORE_BEST, "{}");
      var parsed = JSON.parse(raw);
      return parsed && typeof parsed === "object" ? parsed : {};
    } catch (e) {
      return {};
    }
  }

  function bestKey(meta) {
    return (meta && (meta.id || meta.label)) || "custom";
  }

  function noteBest(meta, score) {
    if (!meta) return false;
    var key = bestKey(meta);
    var best = readBest();
    var prev = Number(best[key]) || 0;
    if (score > prev) {
      best[key] = score;
      writeStore(STORE_BEST, JSON.stringify(best));
      return true;
    }
    return false;
  }

  /* ---------------- 初始化 ---------------- */

  function init() {
    app.sound = new Zuma.Sound();
    app.view = new Zuma.View({
      onFire: onFire,
      onSelect: onSelect,
      onLauncherClick: onLauncherClick,
      onBallClick: onBallClick,
      onHistoryPreview: onHistoryPreview,
      onLevelPick: onLevelPick,
      onColorPick: onColorPick,
      onPreviewReturn: returnToCurrent,
      onInsert: function () { app.sound.play("insert"); },
      onReverse: function () { app.sound.play("insert"); },
      onEliminate: function (ev) { app.sound.play("pop", ev.round || 1); },
      onSwap: function () { app.sound.play("swap"); },
      onUndo: function () { app.sound.play("undo"); },
      onHint: function () { app.sound.play("hint"); },
      onDrawerClose: function () { /* 预览保留，由返回按钮结束 */ }
    });

    app.view.els.board.tabIndex = 0;
    app.view.els.board.setAttribute("aria-label", "彩球游戏区域");
    app.view.populateLevels(samples, DEFAULT_SAMPLE);
    var snd = soundPref();
    app.sound.setEnabled(snd);
    app.view.setSoundState(snd);
    app.view.renderHistory([], -1);

    bindUi();
    var initial = sampleById(DEFAULT_SAMPLE) || samples[0];
    if (initial) loadSample(initial.id);
    else app.view.toast("没有找到内置样例", "error");
  }

  function bindUi() {
    var view = app.view;

    doc.addEventListener("click", function (e) {
      var actionEl = e.target.closest("[data-action]");
      if (actionEl) {
        e.preventDefault();
        dispatchAction(actionEl.dataset.action);
        return;
      }
      if (e.target.closest("#level-button")) {
        e.preventDefault();
        openLevelModal();
        return;
      }
      if (e.target.closest('[data-open="random-modal"]')) {
        root.setTimeout(prepareRandom, 0);
      }
      if (e.target.closest('[data-open="custom-modal"]')) {
        root.setTimeout(function () {
          view.setCustomError(null);
          view.els.customText.focus();
        }, 0);
      }
    });

    doc.addEventListener("pointerdown", function (e) {
      if (!view.colorPickerOpen) return;
      if (e.target.closest("#color-popover") || e.target.closest("#launcher") ||
          e.target.closest(".slot")) return;
      view.hideColorPicker();
    }, true);

    doc.addEventListener("keydown", onKeyDown);

    doc.getElementById("confirm-cancel").addEventListener("click", function () {
      app.pendingLoad = null;
      view.closeModal("confirm-modal");
      app.state = "aim";
      view.focusBoard();
      if (app.controller && app.controller.status !== "playing") scheduleResult();
    });

    doc.getElementById("confirm-ok").addEventListener("click", function () {
      var pending = app.pendingLoad;
      app.pendingLoad = null;
      view.closeModal("confirm-modal");
      if (pending) loadScenario(pending.scenario, pending.meta);
    });

    doc.getElementById("custom-load").addEventListener("click", loadCustom);
    doc.getElementById("random-again").addEventListener("click", function () {
      doc.getElementById("random-seed").value = String(Date.now());
      generateRandom();
      app.sound.play("click");
    });
    doc.getElementById("random-copy").addEventListener("click", copyRandom);
    doc.getElementById("random-load").addEventListener("click", loadRandom);
    doc.getElementById("random-seed").addEventListener("change", function () {
      generateRandom();
    });
    ["random-colors", "random-track", "random-queue", "random-special", "random-limit"]
      .forEach(function (id) {
        doc.getElementById(id).addEventListener("change", function () {
          generateRandom();
        });
      });

    doc.getElementById("pause-resume").addEventListener("click", resumePause);
    doc.getElementById("pause-restart").addEventListener("click", function () {
      view.closeModal("pause-modal");
      app.state = "aim";
      restart();
    });
    doc.getElementById("pause-levels").addEventListener("click", function () {
      view.closeModal("pause-modal");
      app.state = "aim";
      openLevelModal();
    });
    doc.getElementById("pause-sound").addEventListener("click", toggleSound);

    doc.getElementById("result-undo").addEventListener("click", function () {
      view.closeModal("result-modal");
      app.state = "aim";
      doUndo();
    });
    doc.getElementById("result-restart").addEventListener("click", function () {
      view.closeModal("result-modal");
      app.state = "aim";
      restart();
    });
    doc.getElementById("result-levels").addEventListener("click", function () {
      view.closeModal("result-modal");
      app.state = "aim";
      openLevelModal();
    });

    doc.getElementById("range-confirm").addEventListener("click", confirmReverse);
    doc.getElementById("range-cancel").addEventListener("click", cancelReverse);
  }

  /* ---------------- 载入局面 ---------------- */

  function loadSample(id) {
    var sample = sampleById(id);
    if (!sample) return;
    var parsed = parseSample(sample);
    if (parsed.error) {
      app.view.toast("样例解析失败：" + parsed.error, "error");
      return;
    }
    requestLoad(parsed.scenario, {
      id: sample.id,
      title: sample.title,
      label: sample.num + " · " + sample.title
    });
  }

  function loadScenario(scenario, meta) {
    app.loadToken++;
    clearTimeout(app.resultTimer);
    app.resultTimer = 0;
    var view = app.view;
    app.controller = new Zuma.Controller(scenario, meta);
    app.baseScenario = {
      colors: scenario.colors,
      track: scenario.track,
      queue: scenario.queue,
      limit: scenario.limit || 0
    };
    app.baseMeta = meta;
    app.levelId = meta.id || null;
    app.selection = Math.floor(app.controller.engine.trackSize / 2);
    app.previewIndex = null;
    app.chainPeak = 0;
    app.pendingLoad = null;
    app.state = "aim";
    app.reverse = { a: null, b: null };

    view.hideColorPicker();
    view.hideHint();
    view.clearReverseRange();
    view.clearPreview(app.controller.snapshot());
    ["level-modal", "custom-modal", "random-modal", "pause-modal",
      "result-modal", "confirm-modal", "help-modal"].forEach(function (id) {
      view.closeModal(id);
    });
    view.els.previewBanner.hidden = true;
    view.els.board.classList.remove("is-preview");
    view.setLevelLabel(meta.label || meta.title || "自选局面");
    view.populateLevels(samples, app.levelId || "");
    view.renderHistory([], -1);
    view.setCustomError(null);

    var snap = app.controller.snapshot();
    view.render(snap, { chainPeak: 0 });
    view.setSelected(app.selection, { reveal: true });
    view.focusBoard();
    view.announce("已载入 " + (meta.label || meta.title || "自选局面"));

    if (snap.status !== "playing") scheduleResult();
  }

  function requestLoad(scenario, meta) {
    var view = app.view;
    if (view.busy) {
      view.skip();
      view.toast("动画进行中，请稍候", "error");
      return;
    }
    if (app.state === "preview") returnToCurrent();
    if (app.state === "reverse") cancelReverse();
    if (view.colorPickerOpen) view.hideColorPicker();
    if (view.els.historyDrawer.classList.contains("is-open")) view.closeDrawer();
    if (app.controller && app.controller.history.length > 0) {
      app.pendingLoad = { scenario: scenario, meta: meta };
      app.state = "aim";
      ["level-modal", "custom-modal", "random-modal", "pause-modal",
        "result-modal"].forEach(function (id) { view.closeModal(id); });
      doc.getElementById("confirm-text").textContent =
        "载入「" + (meta.label || meta.title || "新局面") +
        "」会结束当前对局，历史与撤销记录将清空。";
      view.openModal("confirm-modal");
      return;
    }
    loadScenario(scenario, meta);
  }

  function restart() {
    if (!app.controller) return;
    loadScenario(app.baseScenario, app.baseMeta);
  }

  function loadCustom() {
    var view = app.view;
    var parsed = Zuma.parseInput(view.els.customText.value);
    if (!parsed.ok) {
      view.setCustomError("解析失败：" + parsed.error);
      app.sound.play("error");
      return;
    }
    view.setCustomError(null);
    requestLoad({
      colors: parsed.colors,
      track: parsed.track,
      queue: parsed.queue,
      limit: parsed.limit || 0
    }, { id: null, title: "自选局面", label: "自选局面" });
  }

  function prepareRandom() {
    var seedEl = doc.getElementById("random-seed");
    if (!seedEl.value.trim()) seedEl.value = String(Date.now());
    generateRandom();
  }

  function generateRandom() {
    var view = app.view;
    var seed = doc.getElementById("random-seed").value.trim() || String(Date.now());
    doc.getElementById("random-seed").value = seed;
    var scenario = Zuma.randomScenario({
      seed: seed,
      colorCount: Number(doc.getElementById("random-colors").value),
      trackLength: Number(doc.getElementById("random-track").value),
      queueLength: Number(doc.getElementById("random-queue").value),
      specialRate: Number(doc.getElementById("random-special").value) / 100,
      limit: Number(doc.getElementById("random-limit").value)
    });
    app.random = scenario;
    app.randomText = Zuma.scenarioText(scenario);
    view.setRandomPreview(app.randomText);
  }

  function copyRandom() {
    if (!app.randomText) generateRandom();
    var text = app.randomText;
    var view = app.view;
    if (root.navigator && root.navigator.clipboard && root.navigator.clipboard.writeText) {
      root.navigator.clipboard.writeText(text).then(function () {
        view.toast("已复制局面输入", "ok");
      }).catch(function () { fallbackCopy(text); });
    } else {
      fallbackCopy(text);
    }
  }

  function fallbackCopy(text) {
    var area = doc.createElement("textarea");
    area.value = text;
    area.style.position = "fixed";
    area.style.opacity = "0";
    doc.body.appendChild(area);
    area.select();
    var ok = false;
    try { ok = doc.execCommand("copy"); } catch (e) { ok = false; }
    area.remove();
    app.view.toast(ok ? "已复制局面输入" : "复制失败，请手动选择文本", ok ? "ok" : "error");
  }

  function loadRandom() {
    generateRandom();
    var seed = app.random.seed;
    requestLoad({
      colors: app.random.colors,
      track: app.random.track,
      queue: app.random.queue,
      limit: app.random.limit || 0
    }, { id: null, title: "随机局面", label: "随机 · 种子 " + seed });
  }

  /* ---------------- 交互守卫 ---------------- */

  function ensureLive() {
    var view = app.view;
    if (view.colorPickerOpen) view.hideColorPicker();
    if (app.state === "preview") returnToCurrent();
    if (app.state === "reverse") cancelReverse();
    if (view.els.historyDrawer.classList.contains("is-open")) view.closeDrawer();
  }

  function canAct() {
    var view = app.view;
    if (!app.controller) return false;
    if (view.busy || app.state === "animating") {
      view.skip();
      return false;
    }
    if (view.modalOpen || app.state === "paused" || app.state === "result") return false;
    if (view.els.historyDrawer.classList.contains("is-open")) view.closeDrawer();
    if (app.state === "preview") returnToCurrent();
    if (app.state === "reverse") return false;
    if (app.controller.status !== "playing") {
      view.toast("对局已结束，可以撤销最后一步或重新开始", "error");
      return false;
    }
    return true;
  }

  function actionError(err) {
    var message = err && err.message ? err.message : "操作失败";
    app.view.toast(message, "error");
    app.sound.play("error");
  }

  /* ---------------- 发射与道具 ---------------- */

  function fireAt(pos, color) {
    if (!canAct()) return;
    var view = app.view;
    var front = app.controller.engine.queue[0];
    if (!front) {
      view.toast("待发射队列为空", "error");
      return;
    }
    if (front.color === "?" && !color) {
      openColorPicker();
      return;
    }
    var turn;
    var token = app.loadToken;
    try {
      turn = app.controller.shoot(pos, color);
    } catch (err) {
      actionError(err);
      return;
    }
    view.hideColorPicker();
    app.state = "animating";
    app.selection = clamp(pos, 0, app.controller.engine.trackSize);
    view.setSelected(app.selection);
    view.playTurn(turn, app.controller).then(function () {
      if (token === app.loadToken) finishTurn(turn);
    });
  }

  function finishTurn(turn) {
    var view = app.view;
    var controller = app.controller;
    app.state = "aim";
    app.previewIndex = null;
    app.chainPeak = controller.chainPeak;
    var snap = controller.snapshot();
    app.selection = clamp(app.selection, 0, snap.track.length);
    view.render(snap, { chainPeak: app.chainPeak });
    view.setSelected(app.selection);
    view.renderHistory(controller.history, controller.history.length - 1);
    view.setHistoryActive(controller.history.length - 1);
    if (turn && turn.label) view.announce(turn.label);
    if (snap.status === "win" || snap.status === "lose") scheduleResult();
  }

  function doUndo() {
    var view = app.view;
    if (!app.controller) return;
    if (view.busy) { view.skip(); return; }
    ensureLive();
    if (!app.controller.engine.undoAvailable) {
      view.toast("没有可撤销的操作", "error");
      app.sound.play("error");
      return;
    }
    view.closeModal("result-modal");
    view.closeModal("pause-modal");
    app.state = "animating";
    var token = app.loadToken;
    var turn = app.controller.undo();
    if (!turn) {
      app.state = "aim";
      view.toast("没有可撤销的操作", "error");
      return;
    }
    view.playTurn(turn, app.controller).then(function () {
      if (token === app.loadToken) finishTurn(turn);
    });
  }

  function doSwap() {
    var view = app.view;
    if (!canAct()) return;
    if (app.controller.engine.queueBalls.length < 2) {
      view.toast("队列不足两球，无法交换", "error");
      app.sound.play("error");
      return;
    }
    var turn;
    var token = app.loadToken;
    try { turn = app.controller.swap(); }
    catch (err) { actionError(err); return; }
    app.state = "animating";
    view.playTurn(turn, app.controller).then(function () {
      if (token === app.loadToken) finishTurn(turn);
    });
  }

  function doHint() {
    var view = app.view;
    if (!canAct()) return;
    if (app.controller.engine.queueBalls.length === 0) {
      view.toast("待发射队列为空", "error");
      return;
    }
    var turn = app.controller.hint();
    var token = app.loadToken;
    app.state = "animating";
    view.playTurn(turn, app.controller).then(function () {
      if (token !== app.loadToken) return;
      if (turn.hintPos >= 0) {
        app.selection = turn.hintPos;
      } else if (app.controller.engine.queueBalls[0] &&
                 app.controller.engine.queueBalls[0].color === "?") {
        view.toast("队首是变色球，请先选择颜色再发射", "error");
      } else {
        view.toast("没有可直接消除的位置，试试交换或倒退", "error");
        app.sound.play("error");
      }
      finishTurn(turn);
    });
  }

  function doReverse(i, j) {
    var view = app.view;
    if (!canAct()) return;
    var turn;
    var token = app.loadToken;
    try { turn = app.controller.reverse(i, j); }
    catch (err) { actionError(err); return; }
    app.state = "animating";
    app.selection = clamp(i, 0, app.controller.engine.trackSize);
    view.showRangeBar(false);
    view.setReverseMode(false);
    view.playTurn(turn, app.controller).then(function () {
      if (token === app.loadToken) finishTurn(turn);
    });
  }

  /* ---------------- 倒退区间选择 ---------------- */

  function enterReverse() {
    var view = app.view;
    if (!app.controller) return;
    if (view.busy) { view.skip(); return; }
    ensureLive();
    if (app.controller.status !== "playing") {
      view.toast("对局已结束，无法倒退", "error");
      return;
    }
    if (app.controller.engine.trackSize < 2) {
      view.toast("轨道不足两颗球，无法倒退", "error");
      return;
    }
    app.state = "reverse";
    app.reverse = { a: null, b: null };
    view.setReverseMode(true);
    view.showRangeBar(true);
    view.setRangeLabel("选择两颗球作为反转区间");
    view.showReverseRange(null);
    view.announce("倒退模式：依次点击两颗彩球，再确认反转");
  }

  function pickReverseEndpoint(index) {
    if (app.state !== "reverse" || !app.controller) return;
    var n = app.controller.engine.trackSize;
    index = clamp(index, 0, n - 1);
    if (app.reverse.a == null) {
      app.reverse.a = index;
    } else if (app.reverse.b == null) {
      app.reverse.b = index;
    } else {
      app.reverse.a = index;
      app.reverse.b = null;
    }
    app.view.showReverseRange(app.reverse.a, app.reverse.b);
    app.view.setRangeLabel(reverseLabel());
    app.view.announce(reverseLabel());
  }

  function reverseLabel() {
    var a = app.reverse.a, b = app.reverse.b;
    if (a == null) return "选择两颗球作为反转区间";
    if (b == null) return "起点 #" + a + "，再选终点";
    var i = Math.min(a, b), j = Math.max(a, b);
    if (i === j) return "区间至少需要两颗球";
    return "反转 [" + i + ", " + j + "]";
  }

  function confirmReverse() {
    if (app.state !== "reverse") return;
    var a = app.reverse.a, b = app.reverse.b;
    if (a == null || b == null) {
      app.view.toast("请先选择区间起点和终点", "error");
      return;
    }
    var i = Math.min(a, b), j = Math.max(a, b);
    if (i === j) {
      app.view.toast("反转区间至少需要两颗球", "error");
      return;
    }
    cancelReverse(true);
    doReverse(i, j);
  }

  function cancelReverse(silent) {
    if (app.state === "reverse") app.state = "aim";
    app.reverse = { a: null, b: null };
    app.view.clearReverseRange();
    app.view.setRangeLabel("反转 [0, 0]");
    if (!silent) app.view.focusBoard();
  }

  /* ---------------- 颜色选择 ---------------- */

  function openColorPicker() {
    var view = app.view;
    if (!app.controller) return;
    if (view.busy) { view.skip(); return; }
    var front = app.controller.engine.queue[0];
    if (!front || front.color !== "?") {
      view.toast("队首不是变色球", "error");
      return;
    }
    view.showColorPicker(app.controller.engine.colors);
    view.announce("请选择变色球颜色");
  }

  function onColorPick(color) {
    app.view.hideColorPicker();
    fireAt(app.selection, color);
  }

  /* ---------------- 暂停 / 历史 / 弹窗 ---------------- */

  function doPause() {
    var view = app.view;
    if (!app.controller) return;
    if (view.busy) { view.skip(); return; }
    ensureLive();
    if (view.modalOpen || app.state === "result") return;
    app.state = "paused";
    view.setSoundState(app.sound.enabled);
    view.openModal("pause-modal");
  }

  function resumePause() {
    app.view.closeModal("pause-modal");
    app.state = "aim";
    app.view.focusBoard();
  }

  function openHistory() {
    var view = app.view;
    if (!app.controller) return;
    if (view.busy) { view.skip(); return; }
    if (app.state === "reverse") cancelReverse();
    view.hideColorPicker();
    var active = app.previewIndex == null
      ? app.controller.history.length - 1 : app.previewIndex;
    view.renderHistory(app.controller.history, active);
    view.openDrawer();
  }

  function onHistoryPreview(index) {
    var view = app.view;
    if (!app.controller) return;
    if (view.busy) { view.skip(); return; }
    var turn = app.controller.history[index];
    if (!turn) return;
    app.previewIndex = index;
    app.state = "preview";
    view.hideColorPicker();
    view.showPreview(app.controller.snapshotAt(index),
      "历史 #" + (index + 1) + " · " + turn.label);
    view.setHistoryActive(index);
    view.closeDrawer();
    view.announce("只读回看第 " + (index + 1) + " 步：" + turn.label);
  }

  function returnToCurrent() {
    var view = app.view;
    if (!app.controller) return;
    app.previewIndex = null;
    app.state = "aim";
    view.clearPreview(app.controller.snapshot());
    view.renderHud(app.controller.snapshot(), { chainPeak: app.controller.chainPeak });
    view.setHistoryActive(app.controller.history.length - 1);
    view.setSelected(app.selection);
    view.focusBoard();
    view.announce("已返回当前局面");
  }

  function openLevelModal() {
    var view = app.view;
    if (view.busy) { view.skip(); return; }
    if (app.state === "preview") returnToCurrent();
    if (app.state === "reverse") cancelReverse();
    view.hideColorPicker();
    view.closeModal("pause-modal");
    view.closeModal("result-modal");
    app.state = "aim";
    view.populateLevels(samples, app.levelId || "");
    view.openModal("level-modal");
  }

  function onLevelPick(id) {
    loadSample(id);
  }

  function scheduleResult() {
    var controller = app.controller;
    clearTimeout(app.resultTimer);
    app.resultTimer = root.setTimeout(function () {
      if (app.controller !== controller) return;
      if (app.state === "animating") return;
      var snap = controller.snapshot();
      if (snap.status === "playing") return;
      var win = snap.status === "win";
      var newBest = win && noteBest(app.baseMeta, snap.score);
      var sub;
      if (win) {
        sub = "彩球全部消除，漂亮的一局。" +
          (newBest ? " 本关新纪录！" : "");
      } else if (snap.limit > 0 && snap.moves >= snap.limit) {
        sub = "步数达到上限 " + snap.limit + "，可以撤销最后一步或重新开始。";
      } else {
        sub = "待发射队列已耗尽，轨道还剩 " + snap.track.length +
          " 颗球。撤销最后一步还能再试。";
      }
      app.state = "result";
      app.view.showResult({
        status: snap.status,
        score: snap.score,
        moves: snap.moves,
        chainPeak: controller.chainPeak,
        cleared: controller.clearedCount(),
        undoAvailable: snap.undoAvailable,
        sub: sub
      });
      app.sound.play(win ? "win" : "lose");
    }, 300);
  }

  /* ---------------- 输入 ---------------- */

  function onSelect(pos) {
    var view = app.view;
    if (!app.controller || view.busy || view.modalOpen) return;
    if (app.state === "paused" || app.state === "result" ||
        app.state === "preview") return;
    var n = app.controller.engine.trackSize;
    pos = clamp(pos, 0, n);
    app.selection = pos;
    view.setSelected(pos);
  }

  function onFire(pos) {
    if (app.state === "preview") {
      returnToCurrent();
      return;
    }
    if (app.state === "reverse") {
      pickReverseEndpoint(clamp(pos, 0,
        app.controller ? app.controller.engine.trackSize - 1 : 0));
      return;
    }
    app.selection = pos;
    fireAt(pos);
  }

  function onBallClick(index) {
    if (!app.controller) return;
    if (app.state === "reverse") {
      pickReverseEndpoint(index);
      return;
    }
    if (app.view.busy || app.view.modalOpen) return;
    if (app.state === "paused" || app.state === "result" ||
        app.state === "preview") return;
    app.selection = clamp(index, 0, app.controller.engine.trackSize);
    app.view.setSelected(app.selection);
    app.view.announce("已选中第 " + app.selection + " 个插入位置");
  }

  function onLauncherClick() {
    if (!app.controller) return;
    if (app.view.busy) { app.view.skip(); return; }
    if (app.state === "preview") { returnToCurrent(); return; }
    if (app.state !== "aim") return;
    var front = app.controller.engine.queue[0];
    if (front && front.color === "?") openColorPicker();
    else fireAt(app.selection);
  }

  function dispatchAction(action) {
    switch (action) {
      case "history":
        if (app.view.els.historyDrawer.classList.contains("is-open"))
          app.view.closeDrawer();
        else openHistory();
        break;
      case "sound": toggleSound(); break;
      case "pause":
        if (app.state === "paused") resumePause();
        else doPause();
        break;
      case "help":
        app.view.openModal("help-modal");
        break;
      case "undo": doUndo(); break;
      case "hint": doHint(); break;
      case "swap": doSwap(); break;
      case "reverse":
        if (app.state === "reverse") cancelReverse();
        else enterReverse();
        break;
      default: break;
    }
  }

  function toggleSound() {
    var on = app.sound.toggle();
    app.view.setSoundState(on);
    writeStore(STORE_SOUND, on ? "1" : "0");
    if (on) app.sound.play("click");
  }

  function onKeyDown(e) {
    if (e.defaultPrevented) return;
    var view = app.view;
    var key = e.key;
    var target = e.target;
    var tag = (target.tagName || "").toLowerCase();
    var typing = tag === "input" || tag === "textarea" || tag === "select" ||
      target.isContentEditable;

    if (key === "Escape") {
      if (view.colorPickerOpen) {
        view.hideColorPicker();
        view.els.launcher.focus();
        e.preventDefault();
        return;
      }
      if (app.state === "reverse") {
        cancelReverse();
        e.preventDefault();
        return;
      }
      if (view.modalOpen) {
        var top = view.topModal;
        view.closeTopModal();
        if (top === "pause-modal" || top === "result-modal") app.state = "aim";
        e.preventDefault();
        return;
      }
      if (view.els.historyDrawer.classList.contains("is-open")) {
        view.closeDrawer();
        e.preventDefault();
        return;
      }
      if (app.state === "preview") {
        returnToCurrent();
        e.preventDefault();
        return;
      }
      if (app.state === "paused") {
        resumePause();
        e.preventDefault();
      }
      return;
    }

    if (typing) return;
    if (e.ctrlKey || e.metaKey || e.altKey) return;
    if (view.busy) {
      if (key === " " || key === "Enter") {
        e.preventDefault();
        view.skip();
      }
      return;
    }
    if (view.modalOpen) return;
    if (app.state === "paused" || app.state === "result") return;

    if (view.colorPickerOpen) {
      if (/^[1-8]$/.test(key)) {
        var colors = app.controller.engine.colors;
        var idx = Number(key) - 1;
        if (idx < colors.length) {
          onColorPick(colors.charAt(idx));
          e.preventDefault();
        }
      }
      return;
    }

    if (app.state === "preview") {
      if (key === "v" || key === "V") {
        openHistory();
        e.preventDefault();
      }
      return;
    }

    if (!app.controller) return;
    var n = app.controller.engine.trackSize;

    if (key === "ArrowLeft" || key === "ArrowRight" ||
        key === "Home" || key === "End" || key === "1" || key === "0") {
      var step = e.shiftKey ? 5 : 1;
      var pos = (key === "1" || key === "Home") ? 0
        : (key === "0" || key === "End") ? n
        : app.selection + (key === "ArrowLeft" ? -step : step);
      app.selection = clamp(pos, 0, n);
      view.setSelected(app.selection, { reveal: true });
      view.announce("插入位置 " + app.selection + "，共 " + n + " 颗球");
      e.preventDefault();
      return;
    }

    if (key === " " || key === "Enter") {
      e.preventDefault();
      if (app.state === "reverse") pickReverseEndpoint(clamp(app.selection, 0, n - 1));
      else fireAt(app.selection);
      return;
    }

    var lower = key.length === 1 ? key.toLowerCase() : key;
    if (lower === "u") { doUndo(); return; }
    if (lower === "h") { doHint(); return; }
    if (lower === "s") { doSwap(); return; }
    if (lower === "r") {
      if (app.state === "reverse") cancelReverse();
      else enterReverse();
      return;
    }
    if (lower === "v") {
      if (view.els.historyDrawer.classList.contains("is-open")) view.closeDrawer();
      else openHistory();
      return;
    }
    if (lower === "m") { toggleSound(); return; }
    if (lower === "p") {
      if (app.state === "paused") resumePause();
      else doPause();
      return;
    }
    if (key === "?" || key === "F1") {
      e.preventDefault();
      view.openModal("help-modal");
      return;
    }
  }

  root.Zuma = root.Zuma || {};
  root.Zuma.app = app;
  root.Zuma.startApp = init;

  if (doc.readyState === "loading")
    doc.addEventListener("DOMContentLoaded", init);
  else
    init();
})(typeof globalThis !== "undefined" ? globalThis : this);
