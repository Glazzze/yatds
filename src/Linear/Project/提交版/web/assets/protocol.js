/*
 * 祖玛 · 协议工具与会话（阶段 5b）
 *
 * 提供：
 *   - parseInput(text)：解析样例头部 + 指令行（头部校验语义对齐 driver）；
 *   - parseCommand(line)：指令解析（错误短语与 PROTOCOL §3.4 一致）；
 *   - Session：按 driver 的终局/错误规则执行指令，输出与 C++ 纯协议
 *     输出同构的语义 token 序列，供 T5.6 交叉验证逐项比对。
 *
 * 本文件不依赖 DOM，UMD 包装；网页自选局面与 Node 交叉验证共用。
 */
(function (root, factory) {
  if (typeof module === "object" && module.exports)
    module.exports = factory(require("./engine.js"));
  else {
    root.Zuma = root.Zuma || {};
    Object.assign(root.Zuma, factory(root.Zuma));
  }
})(typeof globalThis !== "undefined" ? globalThis : this, function (Zuma) {
  "use strict";

  function trim(s) {
    return String(s).replace(/^\s+|\s+$/g, "");
  }

  function parseIntStrict(tok) {
    if (!/^[0-9]+$/.test(tok)) return null;
    var v = 0;
    for (var i = 0; i < tok.length; i++) {
      v = v * 10 + (tok.charCodeAt(i) - 48);
      if (v > 100000000) return null;
    }
    return v;
  }

  function parseCommand(line) {
    var raw = trim(line);
    if (raw === "" || raw.charAt(0) === "#")
      return { kind: "comment", raw: raw };

    var tok = raw.split(/\s+/);
    var head = tok[0].toUpperCase();
    var cmd = { kind: "invalid", raw: raw, pos: 0, end: 0,
                color: 0, hasColor: false, reason: "" };

    if (head === "SHOOT") {
      cmd.kind = "shoot";
      if (tok.length < 2) {
        cmd.kind = "invalid";
        cmd.reason = "SHOOT 缺少位置参数";
      } else if (parseIntStrict(tok[1]) === null) {
        cmd.kind = "invalid";
        cmd.reason = "SHOOT 位置不是非负整数";
      } else {
        cmd.pos = parseIntStrict(tok[1]);
        if (tok.length === 2) {
          cmd.hasColor = false;
        } else if (tok.length === 3 && tok[2].length === 1 &&
                   /[A-Z]/.test(tok[2])) {
          cmd.color = tok[2];
          cmd.hasColor = true;
        } else {
          cmd.kind = "invalid";
          cmd.reason = "SHOOT 参数格式错误（<pos> 或 <pos> <大写字母>）";
        }
      }
    } else if (head === "REVERSE") {
      cmd.kind = "reverse";
      if (tok.length !== 3) {
        cmd.kind = "invalid";
        cmd.reason = "REVERSE 需要 <i> <j> 两个参数";
      } else if (parseIntStrict(tok[1]) === null ||
                 parseIntStrict(tok[2]) === null) {
        cmd.kind = "invalid";
        cmd.reason = "REVERSE 参数不是非负整数";
      } else {
        cmd.pos = parseIntStrict(tok[1]);
        cmd.end = parseIntStrict(tok[2]);
      }
    } else if (head === "UNDO" && tok.length === 1) {
      cmd.kind = "undo";
    } else if (head === "SWAP" && tok.length === 1) {
      cmd.kind = "swap";
    } else if (head === "HINT" && tok.length === 1) {
      cmd.kind = "hint";
    } else if (head === "HELP" && tok.length === 1) {
      cmd.kind = "help";
    } else if (head === "HELP") {
      cmd.kind = "invalid";
      cmd.reason = "HELP 不接受参数";
    } else if (head === "QUIT" && tok.length === 1) {
      cmd.kind = "quit";
    } else {
      cmd.kind = "invalid";
      cmd.reason = "未知指令或参数数量错误";
    }
    return cmd;
  }

  function validateHeader(header) {
    var colors = header.colors;
    if (colors.length === 0) return "颜色集合为空";
    if (colors.length < 2 || colors.length > 8) return "颜色数量必须为 2–8 种";
    if (header.track.length > 1000 || header.queue.length > 2000)
      return "初始轨道最多 1000 球，队列最多 2000 球";
    for (var i = 0; i < colors.length; i++)
      if (!/[A-Z]/.test(colors.charAt(i))) return "颜色集合含非大写字母";
    for (var a = 0; a < colors.length; a++)
      for (var b = a + 1; b < colors.length; b++)
        if (colors.charAt(a) === colors.charAt(b))
          return "颜色集合含重复字母";
    for (var t = 0; t < header.track.length; t++)
      if (colors.indexOf(header.track.charAt(t)) === -1)
        return "轨道含非法球符号";
    for (var q = 0; q < header.queue.length; q++) {
      var c = header.queue.charAt(q);
      if (colors.indexOf(c) === -1 && c !== "*" && c !== "?")
        return "待发射队列含非法球符号";
    }
    return null;
  }

  /*
   * 解析完整输入文本。返回 { ok, error, colors, track, queue, limit,
   * hasLimit, ready, commands }；commands 为 READY 之后的原始非注释行。
   * 头部之后的指令行不影响网页载入（仅装载局面），但交叉验证需要它们。
   */
  function parseInput(text) {
    var lines = String(text == null ? "" : text).replace(/^﻿/, "")
      .split(/\r?\n/);
    var header = { colors: "", track: "", queue: "", limit: 0,
                   hasLimit: false };
    var phase = 0; // 0 COLORS, 1 TRACK, 2 QUEUE, 3 LIMIT/READY/commands
    var ready = false;
    var commandsStarted = false;
    var commands = [];
    var error = null;

    function fail(msg) {
      error = msg;
      return { ok: false, error: msg, colors: header.colors,
               track: header.track, queue: header.queue,
               limit: header.limit, hasLimit: header.hasLimit,
               ready: ready, commands: commands };
    }

    for (var i = 0; i < lines.length; i++) {
      var t = trim(lines[i]);
      if (t === "" || t.charAt(0) === "#") continue;

      if (phase === 0) {
        if (t.indexOf("COLORS ") === 0) {
          header.colors = trim(t.slice(7));
        } else if (t === "COLORS") {
          return fail("COLORS 行缺少颜色集合");
        } else {
          return fail("首行必须是 COLORS 行");
        }
        phase = 1;
      } else if (phase === 1) {
        if (t.indexOf("TRACK ") === 0) header.track = trim(t.slice(6));
        else if (t === "TRACK") header.track = "";
        else return fail("第二行必须是 TRACK 行");
        phase = 2;
      } else if (phase === 2) {
        if (t.indexOf("QUEUE ") === 0) header.queue = trim(t.slice(6));
        else if (t === "QUEUE") header.queue = "";
        else return fail("第三行必须是 QUEUE 行");
        phase = 3;
      } else {
        if (commandsStarted) {
          commands.push(t);
        } else if (!ready && t.indexOf("LIMIT ") === 0) {
          var lim = parseIntStrict(trim(t.slice(6)));
          if (lim === null || lim <= 0) return fail("LIMIT 行参数不是正整数");
          header.limit = lim;
          header.hasLimit = true;
        } else if (!ready && t === "READY") {
          ready = true;
        } else {
          // 首个指令行：后续全部作为指令（即使之后出现 READY 也按未知
          // 指令处理，与 driver 的行式读取一致）。
          commandsStarted = true;
          commands.push(t);
        }
      }
    }

    if (phase < 3)
      return fail("头部不完整（缺 COLORS/TRACK/QUEUE 行或提前 EOF）");
    var bad = validateHeader(header);
    if (bad) return fail(bad);
    return { ok: true, error: null, colors: header.colors,
             track: header.track, queue: header.queue,
             limit: header.limit, hasLimit: header.hasLimit,
             ready: ready, commands: commands };
  }

  function parseScenario(text) {
    var parsed = parseInput(text);
    return parsed;
  }

  function scenarioToInput(s) {
    var out = "COLORS " + s.colors + "\nTRACK " + s.track +
              "\nQUEUE " + s.queue;
    if (s.limit && s.limit > 0) out += "\nLIMIT " + s.limit;
    return out + "\n";
  }

  /*
   * Session 按 driver 规则执行指令并产生语义 token：
   *   header / elim / track / queue / error / hint / help / final
   * 交叉验证工具把 C++ 纯协议输出解析成同样的 token 序列后逐项比较。
   */
  class Session {
    constructor(scenario) {
      this.engine = new Zuma.Engine({
        colors: scenario.colors,
        track: scenario.track,
        queue: scenario.queue,
        limit: scenario.limit || 0
      });
      this.initialStatus = this.engine.initialStatus;
      this.finalShown = false;
      this.quitDone = false;
    }

    _finalToken() {
      var st = this.engine.status;
      var label = st === "win" ? "win" : st === "lose" ? "lose" :
                  st === "quit" ? "quit" : "playing";
      if (label === "playing") return null;
      return { type: "final", status: label, score: this.engine.score,
               moves: this.engine.moves };
    }

    execute(rawLine) {
      if (this.quitDone) return { cmd: null, echo: false, tokens: [], accepted: false };
      var cmd = parseCommand(rawLine);
      var rec = { cmd: cmd, echo: false, tokens: [], accepted: false };
      if (cmd.kind === "comment") return rec;

      if (cmd.kind === "quit") {
        this.engine.quit();
        this.quitDone = true;
        var ft = this._finalToken();
        if (ft) rec.tokens.push(ft);
        rec.accepted = true;
        return rec;
      }

      rec.echo = cmd.kind !== "hint"; // HINT 的结果行替代回显行

      if (this.engine.status !== "playing" &&
          cmd.kind !== "undo" && cmd.kind !== "help") {
        rec.echo = true; // 终局后的 HINT 也回显原文，再输出 ERROR
        rec.tokens.push({ type: "error", reason: "终局后不接受指令" });
        return rec;
      }

      try {
        if (cmd.kind === "shoot") {
          var result = this.engine.shoot(cmd.pos,
            cmd.hasColor ? cmd.color : 0);
          result.events.forEach(function (ev) {
            rec.tokens.push({
              type: "elim", round: ev.round, left: ev.left,
              count: ev.count, balls: ev.balls, points: ev.points,
              trackAfter: ev.trackAfter
            });
          });
          if (result.events.length === 0)
            rec.tokens.push({ type: "track", value: this.engine.trackString() });
          rec.tokens.push({ type: "queue", value: this.engine.queueString() });
          rec.accepted = true;
        } else if (cmd.kind === "reverse") {
          var rr = this.engine.reverse(cmd.pos, cmd.end);
          rr.events.forEach(function (ev) {
            rec.tokens.push({
              type: "elim", round: ev.round, left: ev.left,
              count: ev.count, balls: ev.balls, points: ev.points,
              trackAfter: ev.trackAfter
            });
          });
          if (rr.events.length === 0)
            rec.tokens.push({ type: "track", value: this.engine.trackString() });
          rec.accepted = true;
        } else if (cmd.kind === "undo") {
          if (!this.engine.undo()) {
            rec.tokens.push({ type: "error", reason: "撤销栈为空" });
          } else {
            rec.tokens.push({ type: "track", value: this.engine.trackString() });
            rec.tokens.push({ type: "queue", value: this.engine.queueString() });
            this.finalShown = false; // 从终局回退可再次打印终局
            rec.accepted = true;
          }
        } else if (cmd.kind === "swap") {
          this.engine.swap();
          rec.tokens.push({ type: "queue", value: this.engine.queueString() });
          rec.accepted = true;
        } else if (cmd.kind === "hint") {
          rec.tokens.push({ type: "hint", value: this.engine.hint() });
          rec.accepted = true;
        } else if (cmd.kind === "help") {
          rec.tokens.push({ type: "help" });
          rec.accepted = true;
        } else {
          rec.tokens.push({ type: "error", reason: cmd.reason });
        }
      } catch (e) {
        if (!(e instanceof Zuma.ProtocolError)) throw e;
        rec.tokens.push({ type: "error", reason: e.message });
      }

      if (this.engine.status !== "playing" && !this.finalShown) {
        var fin = this._finalToken();
        if (fin) {
          rec.tokens.push(fin);
          this.finalShown = true;
        }
      }
      return rec;
    }

    finish() {
      if (this.quitDone || this.finalShown) return [];
      var fin = { type: "final", status: "quit",
                  score: this.engine.score, moves: this.engine.moves };
      this.finalShown = true;
      return [fin];
    }
  }

  /*
   * 运行完整输入，返回与 C++ 纯协议语义同构的 token 列表。
   * header 回显使用 parseInput 的原始头部（装载 settle 前）。
   */
  function runInput(text) {
    var parsed = parseInput(text);
    if (!parsed.ok) return { ok: false, error: parsed.error, tokens: [] };
    var session = new Session(parsed);
    var tokens = [
      { type: "colors", value: parsed.colors },
      { type: "track", value: parsed.track },
      { type: "queue", value: parsed.queue }
    ];
    if (parsed.hasLimit) tokens.push({ type: "limit", value: parsed.limit });
    tokens.push({ type: "ready" });

    // 装载终局与操作终局相同：输出一次终局，仍接受 HELP/UNDO/QUIT。
    if (session.initialStatus !== "playing") {
      var ft = session._finalToken();
      if (ft) tokens.push(ft);
      session.finalShown = true;
    }

    for (var i = 0; i < parsed.commands.length; i++) {
      var rec = session.execute(parsed.commands[i]);
      tokens = tokens.concat(rec.tokens);
      if (session.quitDone) break;
    }
    tokens = tokens.concat(session.finish());
    return { ok: true, tokens: tokens, session: session };
  }

  return {
    parseInput: parseInput,
    parseScenario: parseScenario,
    parseCommand: parseCommand,
    scenarioToInput: scenarioToInput,
    Session: Session,
    runInput: runInput
  };
});