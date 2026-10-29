/*
 * 祖玛 · 轻量音效（阶段 5b）
 *
 * 全部用 Web Audio 合成，不加载外部文件；默认关闭，由用户显式开启。
 * 音效开关只影响声音，不改变任何游戏状态。
 */
(function (root) {
  "use strict";

  class Sound {
    constructor() {
      this.enabled = false;
      this.ctx = null;
      this.active = 0;
    }

    setEnabled(on) {
      this.enabled = !!on;
      if (this.enabled) this._ensure();
      else this.stopAll();
    }

    toggle() {
      this.setEnabled(!this.enabled);
      return this.enabled;
    }

    _ensure() {
      if (this.ctx) {
        if (this.ctx.state === "suspended") this.ctx.resume();
        return this.ctx;
      }
      var Ctx = root.AudioContext || root.webkitAudioContext;
      if (!Ctx) return null;
      try {
        this.ctx = new Ctx();
      } catch (e) {
        return null;
      }
      return this.ctx;
    }

    _tone(freq, duration, type, gainValue, delay) {
      if (!this.enabled || this.active >= 3) return;
      var ctx = this._ensure();
      if (!ctx) return;
      var t0 = ctx.currentTime + (delay || 0);
      var osc = ctx.createOscillator();
      var gain = ctx.createGain();
      osc.type = type || "sine";
      osc.frequency.setValueAtTime(freq, t0);
      gain.gain.setValueAtTime(0.0001, t0);
      gain.gain.exponentialRampToValueAtTime(gainValue || 0.12, t0 + 0.01);
      gain.gain.exponentialRampToValueAtTime(0.0001, t0 + duration);
      osc.connect(gain).connect(ctx.destination);
      this.active++;
      osc.onended = () => { this.active = Math.max(0, this.active - 1); };
      osc.start(t0);
      osc.stop(t0 + duration + 0.02);
    }

    play(name, chain) {
      if (!this.enabled) return;
      switch (name) {
        case "insert":
          this._tone(210, 0.06, "sine", 0.10);
          break;
        case "pop":
          this._tone(520 + Math.min(chain || 0, 6) * 80, 0.09, "sine", 0.12);
          break;
        case "swap":
          this._tone(280, 0.06, "triangle", 0.08);
          this._tone(340, 0.07, "triangle", 0.08, 0.055);
          break;
        case "undo":
          this._tone(340, 0.07, "sine", 0.09);
          this._tone(260, 0.09, "sine", 0.09, 0.06);
          break;
        case "error":
          this._tone(180, 0.09, "square", 0.05);
          break;
        case "hint":
          this._tone(660, 0.08, "sine", 0.06);
          break;
        case "win":
          this._tone(523.25, 0.12, "sine", 0.10, 0);
          this._tone(659.25, 0.12, "sine", 0.10, 0.12);
          this._tone(783.99, 0.20, "sine", 0.10, 0.24);
          break;
        case "lose":
          this._tone(330, 0.16, "sine", 0.07, 0);
          this._tone(247, 0.22, "sine", 0.06, 0.14);
          break;
        case "click":
          this._tone(440, 0.035, "sine", 0.04);
          break;
        default:
          break;
      }
    }

    stopAll() {
      if (!this.ctx) return;
      try {
        this.ctx.close();
      } catch (e) { /* ignore */ }
      this.ctx = null;
      this.active = 0;
    }
  }

  root.Zuma = root.Zuma || {};
  root.Zuma.Sound = Sound;
})(typeof globalThis !== "undefined" ? globalThis : this);