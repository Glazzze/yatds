#!/usr/bin/env node
/*
 * 网页静态一致性检查（不依赖浏览器）
 *
 * 校验 index.html 的 id、脚本顺序、资源引用、SVG symbol、标签配对，
 * 以及所有网页脚本的语法与 CSS 大括号平衡。用于补足 DOM 冒烟无法
 * 覆盖的结构性错误；不验证视觉布局。
 */
"use strict";

const fs = require("fs");
const path = require("path");
const vm = require("vm");

const ROOT = path.resolve(__dirname, "..");
const WEB = path.join(ROOT, "web");
const read = (p) => fs.readFileSync(p, "utf8");
const rel = (p) => path.relative(ROOT, p);

let passed = 0;
let failed = 0;

function check(label, ok, detail) {
  if (ok) {
    passed++;
    console.log(`PASS ${label}`);
  } else {
    failed++;
    console.log(`FAIL ${label}${detail ? "：" + detail : ""}`);
  }
}

const htmlPath = path.join(WEB, "index.html");
const html = read(htmlPath);

/* 1. 必需文件 */
const required = [
  "index.html",
  "assets/app.css",
  "assets/engine.js",
  "assets/protocol.js",
  "assets/samples-data.js",
  "assets/audio.js",
  "assets/controller.js",
  "assets/view.js",
  "assets/app.js"
];
const missingFiles = required.filter((p) => !fs.existsSync(path.join(WEB, p)));
check("网页必需文件齐全", missingFiles.length === 0, missingFiles.join(", "));

/* 2. id 唯一且被引用的 id 均存在 */
const ids = [...html.matchAll(/\bid="([^"]+)"/g)].map((m) => m[1]);
const idSet = new Set(ids);
const dupIds = [...new Set(ids.filter((id, i) => ids.indexOf(id) !== i))];
check(`HTML id 唯一（${ids.length} 个）`, dupIds.length === 0, dupIds.join(", "));

const view = read(path.join(WEB, "assets/view.js"));
const idsBlock = view.match(/var IDS = \[([\s\S]*?)\];/);
const viewIds = idsBlock
  ? [...idsBlock[1].matchAll(/"([^"]+)"/g)].map((m) => m[1])
  : [];
check("view.js IDS 列表可解析", viewIds.length > 0);
const missingViewIds = viewIds.filter((id) => !idSet.has(id));
check("view.js IDS 均存在于 HTML", missingViewIds.length === 0,
  missingViewIds.join(", "));

const jsFiles = required.filter((p) => p.endsWith(".js"));
const literalIds = new Set();
for (const file of jsFiles) {
  const src = read(path.join(WEB, file));
  for (const m of src.matchAll(/getElementById\(["']([^"']+)["']\)/g))
    literalIds.add(m[1]);
}
const missingLiteral = [...literalIds].filter((id) => !idSet.has(id));
check(`getElementById 字面量均存在（${literalIds.size} 个）`,
  missingLiteral.length === 0, missingLiteral.join(", "));

/* 3. 脚本/样式引用为本地文件且顺序固定 */
const scripts = [...html.matchAll(/<script\s+src="([^"]+)"/g)].map((m) => m[1]);
const expectedScripts = [
  "assets/engine.js",
  "assets/protocol.js",
  "assets/samples-data.js",
  "assets/audio.js",
  "assets/controller.js",
  "assets/view.js",
  "assets/app.js"
];
check("脚本加载顺序固定", scripts.join("|") === expectedScripts.join("|"),
  scripts.join(", "));
const missingScripts = scripts.filter((p) => !fs.existsSync(path.join(WEB, p)));
check("脚本文件均存在", missingScripts.length === 0, missingScripts.join(", "));

const links = [...html.matchAll(/<link\s+[^>]*href="([^"]+)"/g)]
  .map((m) => m[1])
  .filter((p) => !p.startsWith("data:") && !p.startsWith("#"));
const missingLinks = links.filter((p) => !fs.existsSync(path.join(WEB, p)));
check("样式/图标本地引用均存在", missingLinks.length === 0, missingLinks.join(", "));

/* 4. 无外部网络依赖与 ES module/fetch */
const external = [...html.matchAll(/(?:src|href)="((?:https?:)?\/\/[^"]+)"/g)]
  .map((m) => m[1]);
check("页面无外部 CDN/字体/图片", external.length === 0, external.join(", "));
check("未使用 ES module", !/type="module"/.test(html));
const fetchUsers = jsFiles.filter((p) =>
  /\bfetch\s*\(/.test(read(path.join(WEB, p))));
check("网页脚本不依赖 fetch", fetchUsers.length === 0, fetchUsers.join(", "));

/* 5. SVG symbol 引用闭合 */
const symbols = new Set(
  [...html.matchAll(/<symbol\s+id="([^"]+)"/g)].map((m) => m[1]));
const referencedIcons = new Set();
for (const src of [html, ...jsFiles.map((p) => read(path.join(WEB, p)))]) {
  for (const m of src.matchAll(/href="#(i-[a-z-]+)"/g)) referencedIcons.add(m[1]);
  for (const m of src.matchAll(/["']#(i-[a-z-]+)["']/g)) referencedIcons.add(m[1]);
}
const missingSymbols = [...referencedIcons].filter((id) => !symbols.has(id));
check(`SVG 图标引用已定义（${referencedIcons.size} 个）`,
  missingSymbols.length === 0, missingSymbols.join(", "));

/* 6. JS 语法 */
let syntaxBad = [];
for (const file of jsFiles) {
  try {
    new vm.Script(read(path.join(WEB, file)), { filename: file });
  } catch (err) {
    syntaxBad.push(`${file}: ${err.message}`);
  }
}
check(`网页脚本语法通过（${jsFiles.length} 个）`, syntaxBad.length === 0,
  syntaxBad.join("; "));

/* 7. CSS 大括号平衡（忽略注释与字符串） */
const css = read(path.join(WEB, "assets/app.css"));
const cssClean = css
  .replace(/\/\*[\s\S]*?\*\//g, "")
  .replace(/"(?:\\.|[^"\\])*"/g, '""')
  .replace(/'(?:\\.|[^'\\])*'/g, "''");
let depth = 0;
let cssBad = 0;
for (const ch of cssClean) {
  if (ch === "{") depth++;
  else if (ch === "}" && --depth < 0) cssBad++;
}
check("app.css 大括号平衡", depth === 0 && cssBad === 0,
  `depth=${depth}, bad=${cssBad}`);

/* 8. HTML 标签配对（忽略注释/doctype/void/自闭合） */
const voidTags = new Set(["area", "base", "br", "col", "embed", "hr", "img",
  "input", "link", "meta", "param", "source", "track", "wbr"]);
const stripped = html.replace(/<!--[\s\S]*?-->/g, "");
const stack = [];
const tagErrors = [];
for (const m of stripped.matchAll(/<(\/?)([A-Za-z][\w:-]*)([^>]*)>/g)) {
  const closing = !!m[1];
  const name = m[2].toLowerCase();
  const tail = m[3];
  if (voidTags.has(name) || /\/\s*$/.test(tail)) continue;
  if (!closing) {
    stack.push(name);
  } else {
    const top = stack.pop();
    if (top !== name) tagErrors.push(`</${name}> 对 ${top || "空栈"}`);
  }
}
if (stack.length) tagErrors.push("未闭合：" + stack.join(", "));
check("HTML 标签配对", tagErrors.length === 0, tagErrors.join("; "));

/* 9. 内嵌样例数量 */
const samples = read(path.join(WEB, "assets/samples-data.js"));
const sampleCount = (samples.match(/"id": "case\d+"/g) || []).length;
check(`内嵌样例 16 个（实际 ${sampleCount}）`, sampleCount === 16);

console.log(`\n静态检查：PASS ${passed} 项，FAIL ${failed} 项`);
process.exit(failed ? 1 : 0);
