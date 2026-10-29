#!/usr/bin/env node
/*
 * 网页 app/view 集成冒烟测试（阶段 5b）。
 * 运行：node tools/web_dom_smoke.js（或 make webdom）
 *
 * 本文件内置一个**极简 DOM shim**（元素/选择器/事件/布局尺寸桩），直接在
 * Node 中按 index.html 的顺序加载 7 个脚本，验证初始化、发射与连锁动画、
 * 结算/撤销、自选局面与变色球、历史预览、REVERSE、暂停/音效/帮助、
 * SWAP/HINT/UNDO、LIMIT 失败、随机生成等主流程。
 *
 * 它不是 jsdom/真实浏览器：不执行 CSS，不校验视觉布局；结论以交互与
 * DOM 状态为主，真实浏览器/缩放手测仍需按 docs/web-test.md 执行。
 */
"use strict";
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const assert = require("assert");

const PROJECT = process.argv[2] || require("path").resolve(__dirname, "..");

/* ------------------------- tiny DOM ------------------------- */
const VOID = new Set(["area","base","br","col","embed","hr","img","input","link","meta","param","source","track","wbr"]);

function decode(s) {
  return String(s).replace(/&amp;/g,"&").replace(/&lt;/g,"<").replace(/&gt;/g,">")
    .replace(/&quot;/g,'"').replace(/&#39;/g,"'").replace(/&nbsp;/g," ");
}

function camel(name) {
  return name.replace(/-([a-z])/g, (_, c) => c.toUpperCase());
}

function makeDataset(el) {
  const target = {};
  return new Proxy(target, {
    get(t, p) { return t[p]; },
    set(t, p, v) {
      t[p] = String(v);
      el.attributes["data-" + String(p).replace(/[A-Z]/g, (m) => "-" + m.toLowerCase())] = String(v);
      return true;
    }
  });
}

function makeStyle() {
  const target = {};
  const proxy = new Proxy(target, {
    get(t, p) {
      if (p === "setProperty") return (k, v) => { t[k] = String(v); };
      if (p === "removeProperty") return (k) => { delete t[k]; };
      if (p === "getPropertyValue") return (k) => t[k] || "";
      if (p in t) return t[p];
      return "";
    },
    set(t, p, v) { t[p] = String(v); return true; }
  });
  return proxy;
}

class TextNode {
  constructor(text) { this.nodeType = 3; this.textContent = text; this.parentNode = null; }
  get textContent() { return this._text || ""; }
  set textContent(v) { this._text = String(v); }
  get childNodes() { return []; }
}

class ClassList {
  constructor(el) { this.el = el; }
  add(...xs) { xs.forEach((x) => this.el._classes.add(x)); }
  remove(...xs) { xs.forEach((x) => this.el._classes.delete(x)); }
  contains(x) { return this.el._classes.has(x); }
  toggle(x, force) {
    if (force === undefined) force = !this.contains(x);
    if (force) this.add(x); else this.remove(x);
    return !!force;
  }
  get value() { return [...this.el._classes].join(" "); }
}

class Element {
  constructor(tagName, owner) {
    this.ownerDocument = owner;
    this.tagName = String(tagName).toUpperCase();
    this.nodeType = 1;
    this.childNodes = [];
    this.parentNode = null;
    this.attributes = Object.create(null);
    this._classes = new Set();
    this.classList = new ClassList(this);
    this.style = makeStyle();
    this.dataset = makeDataset(this);
    this.hidden = false;
    this.disabled = false;
    this.tabIndex = 0;
    this.value = "";
    this.title = "";
    this._listeners = Object.create(null);
    this._id = "";
  }
  get id() { return this._id; }
  set id(v) { this._id = String(v); if (this.ownerDocument) this.ownerDocument._ids.set(this._id, this); }
  get className() { return [...this._classes].join(" "); }
  set className(v) { this._classes = new Set(String(v).split(/\s+/).filter(Boolean)); }
  get children() { return this.childNodes.filter((n) => n.nodeType === 1); }
  get childElementCount() { return this.children.length; }
  get firstElementChild() { return this.children[0] || null; }
  get offsetWidth() { return 100; }
  get clientWidth() { return 760; }
  get clientHeight() { return 132; }
  get scrollHeight() { return 0; }
  get scrollTop() { return this._scrollTop || 0; }
  set scrollTop(v) { this._scrollTop = Number(v) || 0; }
  get scrollLeft() { return this._scrollLeft || 0; }
  set scrollLeft(v) { this._scrollLeft = Number(v) || 0; }
  get offsetParent() {
    if (this.hidden) return null;
    let p = this.parentNode;
    while (p) {
      if (p.hidden) return null;
      p = p.parentNode;
    }
    return this.parentNode || this.ownerDocument.body || null;
  }
  get textContent() {
    return this.childNodes.map((n) => n.textContent || "").join("");
  }
  set textContent(v) {
    this.childNodes.forEach((n) => { n.parentNode = null; });
    this.childNodes = [];
    if (String(v) !== "") this.appendChild(new TextNode(String(v)));
  }
  get innerHTML() { return this._innerHTML || ""; }
  set innerHTML(v) {
    this._innerHTML = String(v);
    this.childNodes.forEach((n) => { n.parentNode = null; });
    this.childNodes = [];
    parseHTML(String(v), this.ownerDocument, this);
  }
  setAttribute(name, value) {
    name = String(name);
    const v = value === undefined ? "" : String(value);
    this.attributes[name] = v;
    if (name === "class") this.className = v;
    else if (name === "id") this.id = v;
    else if (name === "hidden") this.hidden = true;
    else if (name === "disabled") this.disabled = true;
    else if (name === "tabindex") this.tabIndex = Number(v);
    else if (name === "value") this.value = v;
    else if (name === "title") this.title = v;
    else if (name.startsWith("data-")) this.dataset[camel(name.slice(5))] = v;
  }
  getAttribute(name) { return name in this.attributes ? this.attributes[name] : null; }
  hasAttribute(name) { return name in this.attributes; }
  removeAttribute(name) {
    delete this.attributes[name];
    if (name === "hidden") this.hidden = false;
    if (name === "disabled") this.disabled = false;
    if (name === "title") this.title = "";
  }
  appendChild(node) {
    if (node.nodeType === 11) {
      node.childNodes.slice().forEach((c) => this.appendChild(c));
      return node;
    }
    if (node.parentNode) node.parentNode.childNodes = node.parentNode.childNodes.filter((n) => n !== node);
    node.parentNode = this;
    this.childNodes.push(node);
    return node;
  }
  replaceChildren(...nodes) {
    this.childNodes.forEach((n) => { n.parentNode = null; });
    this.childNodes = [];
    nodes.forEach((n) => this.appendChild(n));
  }
  remove() {
    if (!this.parentNode) return;
    this.parentNode.childNodes = this.parentNode.childNodes.filter((n) => n !== this);
    this.parentNode = null;
  }
  focus() { if (this.ownerDocument) this.ownerDocument.activeElement = this; }
  select() {}
  closest(sel) {
    let n = this;
    while (n && n.nodeType === 1) {
      if (matches(n, sel)) return n;
      n = n.parentNode;
    }
    return null;
  }
  querySelector(sel) { return descendants(this).find((n) => matches(n, sel)) || null; }
  querySelectorAll(sel) { return descendants(this).filter((n) => matches(n, sel)); }
  getBoundingClientRect() { return { left: 0, top: 0, right: 120, bottom: 32, width: 120, height: 32 }; }
  addEventListener(type, fn, opts) {
    const capture = !!(opts === true || (opts && opts.capture));
    const ast = this._listeners[type] || (this._listeners[type] = { capture: [], bubble: [] });
    (capture ? ast.capture : ast.bubble).push(fn);
  }
  _dispatchLocal(event, capture) {
    const ast = this._listeners[event.type];
    if (!ast) return;
    const list = capture ? ast.capture : ast.bubble;
    event.currentTarget = this;
    for (const fn of list.slice()) {
      if (event._stopped) break;
      fn.call(this, event);
    }
  }
}

class Fragment extends Element {
  constructor(owner) { super("#fragment", owner); this.nodeType = 11; this.tagName = "#fragment"; }
}

class Document {
  constructor() {
    this.nodeType = 9;
    this._ids = new Map();
    this._listeners = Object.create(null);
    this.activeElement = null;
    this.readyState = "complete";
    this._byId = this._ids;
    this.documentElement = null;
    this.body = null;
    this.head = null;
    this.defaultView = null;
    this.childNodes = [];
    this.parentNode = null;
  }
  appendChild(node) {
    if (node.nodeType === 11) {
      node.childNodes.slice().forEach((c) => this.appendChild(c));
      return node;
    }
    if (node.parentNode) node.parentNode.childNodes = node.parentNode.childNodes.filter((n) => n !== node);
    node.parentNode = this;
    this.childNodes.push(node);
    return node;
  }
  createElement(tag) { return new Element(tag, this); }
  createTextNode(text) { return new TextNode(text); }
  createDocumentFragment() { return new Fragment(this); }
  getElementById(id) { return this._ids.get(String(id)) || null; }
  querySelector(sel) { return this.documentElement ? this.documentElement.querySelector(sel) : null; }
  querySelectorAll(sel) { return this.documentElement ? this.documentElement.querySelectorAll(sel) : []; }
  addEventListener(type, fn, opts) {
    const capture = !!(opts === true || (opts && opts.capture));
    const ast = this._listeners[type] || (this._listeners[type] = { capture: [], bubble: [] });
    (capture ? ast.capture : ast.bubble).push(fn);
  }
  _dispatchLocal(event, capture) {
    const ast = this._listeners[event.type];
    if (!ast) return;
    const list = capture ? ast.capture : ast.bubble;
    event.currentTarget = this;
    for (const fn of list.slice()) {
      if (event._stopped) break;
      fn.call(this, event);
    }
  }
  execCommand() { return true; }
}

function descendants(el) {
  const out = [];
  (function walk(n) {
    for (const c of n.childNodes) {
      if (c.nodeType === 1) { out.push(c); walk(c); }
    }
  })(el);
  return out;
}

function matchCompound(el, sel) {
  sel = sel.trim();
  if (!sel) return false;
  let i = 0;
  const tag = /^[A-Za-z][\w-]*/.exec(sel);
  if (tag) {
    if (el.tagName !== tag[0].toUpperCase()) return false;
    i = tag[0].length;
  }
  while (i < sel.length) {
    if (sel[i] === ".") {
      const m = /^\.([\w-]+)/.exec(sel.slice(i));
      if (!m) return false;
      if (!el.classList.contains(m[1])) return false;
      i += m[0].length;
    } else if (sel[i] === "#") {
      const m = /^#([\w-]+)/.exec(sel.slice(i));
      if (!m) return false;
      if (el.id !== m[1]) return false;
      i += m[0].length;
    } else if (sel[i] === "[") {
      const end = sel.indexOf("]", i);
      if (end < 0) return false;
      let body = sel.slice(i + 1, end).trim();
      let name, op, val;
      const m = /^([\w:-]+)\s*(?:([~|^$*]?=)\s*"?([^"\]]*)"?)?$/.exec(body);
      if (!m) return false;
      name = m[1]; op = m[2]; val = m[3];
      if (!(name in el.attributes)) return false;
      if (op) {
        const actual = el.attributes[name];
        if (op === "=" && actual !== val) return false;
        if (op === "*=" && actual.indexOf(val) < 0) return false;
        if (op === "^=" && actual.indexOf(val) !== 0) return false;
        if (op === "$=" && actual.slice(-val.length) !== val) return false;
      }
      i = end + 1;
    } else if (sel.startsWith(":not(", i)) {
      let depth = 0, j = i + 5;
      for (; j < sel.length; j++) {
        if (sel[j] === "(") depth++;
        else if (sel[j] === ")") { if (depth === 0) break; depth--; }
      }
      if (j >= sel.length) return false;
      const inner = sel.slice(i + 5, j);
      if (matchCompound(el, inner)) return false;
      i = j + 1;
    } else {
      return false;
    }
  }
  return true;
}

function matches(el, sel) {
  return String(sel).split(",").some((s) => matchCompound(el, s));
}

function parseAttrs(str, el) {
  const re = /([^\s=/>]+)(?:\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s"'>`]+)))?/g;
  let m;
  while ((m = re.exec(str))) {
    const name = m[1];
    const val = m[2] !== undefined ? m[2] : m[3] !== undefined ? m[3] : m[4] !== undefined ? m[4] : "";
    el.setAttribute(name, decode(val));
  }
}

function parseHTML(html, doc, root) {
  root = root || doc;
  const stack = [root];
  const tokenRe = /<!--[\s\S]*?-->|<![^>]*>|<\/?[^>]+>|[^<]+/g;
  let m;
  while ((m = tokenRe.exec(html))) {
    const tok = m[0];
    if (tok.startsWith("<!--") || /^<![^>]*>/.test(tok)) continue;
    if (tok.startsWith("</")) {
      const tag = tok.slice(2, -1).trim().toLowerCase();
      for (let i = stack.length - 1; i > 0; i--) {
        if (stack[i].tagName === tag.toUpperCase()) { stack.length = i; break; }
      }
      continue;
    }
    if (tok.startsWith("<")) {
      const st = /^<\s*([A-Za-z][^\s/>]*)([^>]*?)(\/?)>$/.exec(tok);
      if (!st) continue;
      const tag = st[1].toLowerCase();
      const el = doc.createElement(tag);
      parseAttrs(st[2], el);
      stack[stack.length - 1].appendChild(el);
      if (el.id) doc._ids.set(el.id, el);
      if (tag === "html") doc.documentElement = el;
      if (tag === "body") doc.body = el;
      if (tag === "head") doc.head = el;
      if (!st[3] && !VOID.has(tag)) stack.push(el);
      continue;
    }
    const text = decode(tok);
    if (text) stack[stack.length - 1].appendChild(new TextNode(text));
  }
}

function buildDocument(html) {
  const doc = new Document();
  parseHTML(html, doc, doc);
  if (!doc.documentElement && doc.body) doc.documentElement = doc.body;
  doc.activeElement = doc.body;
  return doc;
}

function eventPath(target) {
  const path = [];
  let n = target;
  while (n) { path.push(n); n = n.parentNode || (n === n.ownerDocument ? null : n.ownerDocument); }
  return path;
}

function dispatch(target, event) {
  event.target = target;
  event.defaultPrevented = false;
  event._stopped = false;
  event.preventDefault = () => { event.defaultPrevented = true; };
  event.stopPropagation = () => { event._stopped = true; };
  const path = eventPath(target);
  for (let i = path.length - 1; i >= 0 && !event._stopped; i--)
    if (path[i]._dispatchLocal) path[i]._dispatchLocal(event, true);
  for (let i = 0; i < path.length && !event._stopped; i++)
    if (path[i]._dispatchLocal) path[i]._dispatchLocal(event, false);
  return !event.defaultPrevented;
}

/* ------------------------- load page ------------------------- */
(async function main() {
  const html = fs.readFileSync(path.join(PROJECT, "web/index.html"), "utf8");
  const doc = buildDocument(html);
  assert.ok(doc.getElementById("board"), "board parsed");
  const store = Object.create(null);
  const localStorage = {
    getItem(k) { return k in store ? store[k] : null; },
    setItem(k, v) { store[k] = String(v); },
    removeItem(k) { delete store[k]; }
  };
  const sandbox = {
    console,
    document: doc,
    navigator: { clipboard: { writeText: async () => {} } },
    localStorage,
    matchMedia: () => ({ matches: true }),
    innerWidth: 1200,
    innerHeight: 800,
    setTimeout,
    clearTimeout,
    setInterval,
    clearInterval,
    addEventListener: function () {},
    removeEventListener: function () {},
    AudioContext: undefined,
    webkitAudioContext: undefined
  };
  sandbox.window = sandbox;
  sandbox.globalThis = sandbox;
  doc.defaultView = sandbox;
  const context = vm.createContext(sandbox);
  for (const file of ["engine.js","protocol.js","samples-data.js","audio.js","controller.js","view.js","app.js"]) {
    const code = fs.readFileSync(path.join(PROJECT, "web/assets", file), "utf8");
    vm.runInContext(code, context, { filename: file });
  }
  const Zuma = sandbox.Zuma;
  const app = Zuma.app;
  const view = app.view;
  assert.ok(app && view, "app booted");

  const $ = (id) => doc.getElementById(id);
  const wait = (ms) => new Promise((r) => setTimeout(r, ms));
  const tick = () => new Promise((r) => setImmediate(r));
  async function settle() { for (let i = 0; i < 8; i++) await tick(); }
  async function waitFor(fn, ms, label) {
    const end = Date.now() + (ms || 1500);
    while (Date.now() < end) {
      if (fn()) return;
      await wait(5);
    }
    throw new Error("timeout: " + (label || "condition"));
  }
  function click(el) {
    assert.ok(el, "click target exists");
    dispatch(el, { type: "click" });
  }
  function key(k, target) {
    dispatch(target || doc, { type: "keydown", key: k, shiftKey: false, ctrlKey: false, metaKey: false, altKey: false });
  }
  async function loadCustom(text) {
    if ($("level-modal").hidden) click($("level-button"));
    click($("level-modal").querySelector('[data-open="custom-modal"]'));
    assert.strictEqual($("custom-modal").hidden, false);
    $("custom-text").value = text;
    click($("custom-load"));
    if (!$("confirm-modal").hidden) click($("confirm-ok"));
    await settle();
  }

  /* 初始：case03 */
  assert.strictEqual($("level-label").textContent, "03 · 连锁演示");
  assert.strictEqual($("track-count").textContent, "轨道 6");
  assert.strictEqual($("queue-count").textContent, "1");
  assert.strictEqual($("score-value").textContent, "0");
  assert.strictEqual(app.controller.engine.trackString(), "RRBBRR");
  assert.strictEqual(view.els.trackBalls.childElementCount, 6);
  assert.strictEqual(view.els.trackSlots.childElementCount, 7);
  assert.strictEqual($("board").tabIndex, 0);
  assert.strictEqual($("history-list").textContent.indexOf("还没有操作") >= 0, true);
  assert.strictEqual(app.selection, 3);
  key("1");
  assert.strictEqual(app.selection, 0);
  key("0");
  assert.strictEqual(app.selection, 6);
  key("ArrowLeft");
  key("ArrowLeft");
  assert.strictEqual(app.selection, 4);
  console.log("PASS 初始装载 case03、默认中点与 1/0 首尾选位");

  /* 空格发射 → 连锁 → 胜利结算 */
  key(" ");
  await waitFor(() => app.controller.history.length === 1 && !view.busy, 2000, "shoot done");
  assert.strictEqual(app.controller.status, "win");
  assert.strictEqual(app.controller.score, 110);
  assert.strictEqual($("score-value").textContent, "110");
  assert.strictEqual(view.els.trackBalls.childElementCount, 0);
  assert.strictEqual(view.els.historyList.querySelectorAll(".history-btn").length, 1);
  await waitFor(() => !$("result-modal").hidden, 1500, "result modal");
  assert.strictEqual($("result-title").textContent, "轨道清空！");
  assert.strictEqual($("result-score").textContent, "110");
  assert.strictEqual($("result-chain").textContent, "×2");
  assert.strictEqual($("result-cleared").textContent, "7");
  console.log("PASS 发射 / 连锁动画 / 胜利结算");

  /* 结算撤销 */
  click($("result-undo"));
  await waitFor(() => app.controller.history.length === 2 && !view.busy, 2000, "undo done");
  assert.strictEqual(app.controller.status, "playing");
  assert.strictEqual(app.controller.score, 0);
  assert.strictEqual(app.controller.engine.trackString(), "RRBBRR");
  assert.strictEqual($("result-modal").hidden, true);
  assert.strictEqual($("score-value").textContent, "0");
  assert.strictEqual(view.els.trackBalls.childElementCount, 6);
  assert.strictEqual($("undo-button").disabled, true);
  console.log("PASS 结算撤销恢复局面与历史");

  /* 自选局面：变色球与确认弹窗 */
  click(doc.querySelector('[data-open="custom-modal"]'));
  assert.strictEqual($("custom-modal").hidden, false);
  $("custom-text").value = "COLORS RB\nTRACK RRB\nQUEUE ?R\n";
  click($("custom-load"));
  assert.strictEqual($("confirm-modal").hidden, false);
  click($("confirm-ok"));
  assert.strictEqual(app.controller.engine.queueString(), "?R");
  assert.strictEqual($("custom-modal").hidden, true);
  assert.strictEqual(app.controller.engine.trackString(), "RRB");
  assert.strictEqual($("level-label").textContent, "自选局面");
  assert.strictEqual(view.els.trackBalls.childElementCount, 3);
  key("ArrowLeft"); // selection 3 → 2
  key(" ");
  assert.strictEqual($("color-popover").hidden, false, "color picker open");
  assert.strictEqual(view.els.colorSwatches.querySelectorAll(".swatch").length, 2);
  key("1");
  await waitFor(() => app.controller.history.length === 1 && !view.busy, 2000, "chameleon shot");
  assert.strictEqual(app.controller.engine.trackString(), "B");
  assert.strictEqual(app.controller.score, 30);
  assert.strictEqual($("color-popover").hidden, true);
  console.log("PASS 自选局面 / 变色球选色 / 消除");

  /* 历史抽屉只读预览 */
  click(doc.querySelector('[data-action="history"]'));
  assert.strictEqual($("history-drawer").classList.contains("is-open"), true);
  assert.strictEqual(view.els.historyList.querySelectorAll(".history-btn").length, 1);
  click(view.els.historyList.querySelector(".history-btn"));
  assert.strictEqual(app.state, "preview");
  assert.strictEqual($("preview-banner").hidden, false);
  assert.strictEqual($("board").classList.contains("is-preview"), true);
  click($("preview-return"));
  assert.strictEqual(app.state, "aim");
  assert.strictEqual($("preview-banner").hidden, true);
  assert.strictEqual($("board").classList.contains("is-preview"), false);
  console.log("PASS 历史抽屉 / 只读预览 / 返回当前");

  /* 关卡切换 + 倒退回转 */
  click($("level-button"));
  assert.strictEqual($("level-modal").hidden, false);
  const card12 = doc.querySelector('.level-card[data-id="case12"]');
  assert.ok(card12, "case12 card");
  click(card12);
  assert.strictEqual($("confirm-modal").hidden, false);
  click($("confirm-ok"));
  await waitFor(() => app.controller.engine.trackString() === "RRBBRR" && app.controller.engine.queueString() === "R", 1000, "case12 load");
  assert.strictEqual($("level-label").textContent, "12 · 倒退拼图");
  click(doc.querySelector('[data-action="reverse"]'));
  assert.strictEqual(app.state, "reverse");
  assert.strictEqual($("range-bar").hidden, false);
  assert.strictEqual($("board").classList.contains("is-reverse"), true);
  click(view.els.trackSlots.querySelector('.slot[data-pos="2"]'));
  click(view.els.trackSlots.querySelector('.slot[data-pos="5"]'));
  assert.strictEqual($("range-text").textContent, "反转 [2, 5]");
  click($("range-confirm"));
  await waitFor(() => app.controller.history.length === 1 && !view.busy, 2000, "reverse done");
  assert.strictEqual(app.controller.engine.trackString(), "BB");
  assert.strictEqual(app.controller.score, 40);
  assert.strictEqual(app.state, "aim");
  assert.strictEqual($("range-bar").hidden, true);
  assert.strictEqual($("board").classList.contains("is-reverse"), false);
  console.log("PASS 关卡切换 / REVERSE 区间选择与结算");

  /* 暂停 / 音效 / 帮助 */
  click(doc.querySelector('[data-action="pause"]'));
  assert.strictEqual(app.state, "paused");
  assert.strictEqual($("pause-modal").hidden, false);
  click($("pause-resume"));
  assert.strictEqual(app.state, "aim");
  assert.strictEqual($("pause-modal").hidden, true);
  click($("sound-button"));
  assert.strictEqual($("sound-button").getAttribute("aria-label"), "关闭音效");
  assert.strictEqual(store["zuma.sound"], "1");
  click(doc.querySelector('[data-action="help"]'));
  assert.strictEqual($("help-modal").hidden, false);
  click($("help-modal").querySelector('[data-close="help-modal"]'));
  assert.strictEqual($("help-modal").hidden, true);
  console.log("PASS 暂停 / 音效开关 / 帮助弹窗");

  /* SWAP / HINT / UNDO 按钮链路 */
  await loadCustom("COLORS RB\nTRACK RRBBRR\nQUEUE BR\n");
  assert.strictEqual(app.controller.engine.queueString(), "BR");
  click($("swap-button"));
  await waitFor(() => app.controller.history.length === 1 && !view.busy, 2000, "swap done");
  assert.strictEqual(app.controller.engine.queueString(), "RB");
  assert.strictEqual(app.controller.moves, 1);
  click($("hint-button"));
  await waitFor(() => app.controller.history.length === 2 && !view.busy, 2000, "hint done");
  assert.strictEqual(app.controller.history[1].kind, "hint");
  assert.strictEqual(app.state, "aim");
  click($("undo-button"));
  await waitFor(() => app.controller.history.length === 3 && !view.busy, 2000, "undo done 2");
  assert.strictEqual(app.controller.engine.queueString(), "BR");
  assert.strictEqual(app.controller.moves, 0);
  assert.strictEqual($("undo-button").disabled, true);
  console.log("PASS SWAP / HINT / 撤销按钮链路");

  /* LIMIT 失败结算与重新开始 */
  await loadCustom("COLORS RB\nTRACK RB\nQUEUE RRR\nLIMIT 2\n");
  assert.strictEqual($("limit-chip").hidden, false);
  assert.strictEqual($("limit-chip").textContent, "上限 2");
  key(" ");
  await waitFor(() => app.controller.history.length === 1 && !view.busy, 2000, "limit shot 1");
  assert.strictEqual(app.controller.status, "playing");
  key(" ");
  await waitFor(() => app.controller.history.length === 2 && !view.busy, 2000, "limit shot 2");
  assert.strictEqual(app.controller.status, "lose");
  await waitFor(() => !$("result-modal").hidden, 1500, "lose modal");
  assert.strictEqual($("result-title").textContent, "差一点就清空了");
  assert.ok($("result-sub").textContent.indexOf("步数达到上限 2") >= 0);
  click($("result-restart"));
  await waitFor(() => app.controller.history.length === 0 && $("result-modal").hidden, 1000, "restart");
  assert.strictEqual(app.controller.engine.trackString(), "RB");
  assert.strictEqual(app.controller.engine.queueString(), "RRR");
  assert.strictEqual(app.controller.moves, 0);
  assert.strictEqual(app.state, "aim");
  console.log("PASS LIMIT 失败结算 / 重新开始");

  /* 随机局面生成器预览与载入 */
  click($("level-button"));
  click($("level-modal").querySelector('[data-open="random-modal"]'));
  await wait(20);
  assert.strictEqual($("random-modal").hidden, false);
  assert.ok($("random-preview").textContent.indexOf("COLORS ") === 0);
  assert.ok($("random-preview").textContent.indexOf("TRACK ") >= 0);
  $("random-seed").value = "smoke-test";
  dispatch($("random-seed"), { type: "change" });
  const expectedTrack = app.random.track;
  const expectedQueue = app.random.queue;
  click($("random-load"));
  await settle();
  assert.strictEqual(app.controller.engine.trackString(), expectedTrack);
  assert.strictEqual(app.controller.engine.queueString(), expectedQueue);
  assert.ok($("level-label").textContent.indexOf("随机 · 种子 smoke-test") === 0);
  console.log("PASS 随机局面生成器预览与载入");

  console.log("\nDOM 冒烟：全部通过");
  process.exit(0);
})().catch((err) => {
  console.error(err && err.stack ? err.stack : err);
  process.exit(1);
});
