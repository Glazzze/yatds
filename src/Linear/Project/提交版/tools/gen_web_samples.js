#!/usr/bin/env node
/*
 * 由 inputs/caseNN.txt 生成 web/assets/samples-data.js。
 * 网页在 file:// 下不能 fetch 本地文件，因此样例以 JS 字面量内嵌。
 * 运行：node tools/gen_web_samples.js（或 make web-samples）
 */
"use strict";

const fs = require("fs");
const path = require("path");

const PROJECT = path.resolve(__dirname, "..");
const INPUT = path.join(PROJECT, "inputs");
const OUT = path.join(PROJECT, "web/assets/samples-data.js");

const TITLES = {
  case01: "初见轨道",
  case02: "三连入门",
  case03: "连锁演示",
  case04: "差一点三连",
  case05: "两端落子",
  case06: "撤销再战",
  case07: "连锁回滚",
  case08: "炸弹开路",
  case09: "变色挑战",
  case10: "交换顺序",
  case11: "提示与无解",
  case12: "倒退拼图",
  case13: "步数上限",
  case14: "队列耗尽",
  case15: "最后一球",
  case16: "错误示范"
};

const files = fs.readdirSync(INPUT).filter(f => /^case\d+\.txt$/.test(f)).sort();
const samples = files.map(file => {
  const id = file.replace(/\.txt$/, "");
  const num = id.slice(4);
  return {
    id: id,
    num: num,
    title: TITLES[id] || ("样例 " + num),
    input: fs.readFileSync(path.join(INPUT, file), "utf8")
  };
});

const out = [
  "/*",
  " * 自动生成：node tools/gen_web_samples.js（make web-samples）",
  " * 数据源：inputs/caseNN.txt。请勿手工编辑本文件。",
  " */",
  "(function (root) {",
  '  root.Zuma = root.Zuma || {};',
  "  root.Zuma.samples = " + JSON.stringify(samples, null, 2) + ";",
  "})(typeof globalThis !== \"undefined\" ? globalThis : this);",
  ""
].join("\n");

fs.writeFileSync(OUT, out);
console.log("生成 " + path.relative(PROJECT, OUT) + "（" + samples.length + " 个样例）");