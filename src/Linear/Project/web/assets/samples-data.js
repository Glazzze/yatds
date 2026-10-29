/*
 * 自动生成：node tools/gen_web_samples.js（make web-samples）
 * 数据源：inputs/caseNN.txt。请勿手工编辑本文件。
 */
(function (root) {
  root.Zuma = root.Zuma || {};
  root.Zuma.samples = [
  {
    "id": "case01",
    "num": "01",
    "title": "初见轨道",
    "input": "# case01 普通插入（中间位置，不触发消除）\nCOLORS RGBY\nTRACK RBG\nQUEUE YR\nSHOOT 2\nQUIT"
  },
  {
    "id": "case02",
    "num": "02",
    "title": "三连入门",
    "input": "# case02 三连消除（单轮）：RRB + R → RRR 消除\nCOLORS RB\nTRACK RRB\nQUEUE RB\nSHOOT 2\nQUIT"
  },
  {
    "id": "case03",
    "num": "03",
    "title": "连锁演示",
    "input": "# case03 多级连锁（README 示例）：BBB 消除后 RRRR 再消 → 轨道清空\nCOLORS RB\nTRACK RRBBRR\nQUEUE B\nSHOOT 4"
  },
  {
    "id": "case04",
    "num": "04",
    "title": "差一点三连",
    "input": "# case04 不足三连：插入后仅形成 RR 两连，不消除\nCOLORS RB\nTRACK RBR\nQUEUE RB\nSHOOT 1\nQUIT"
  },
  {
    "id": "case05",
    "num": "05",
    "title": "两端落子",
    "input": "# case05 插入位置边界：先插到 0（左端），再插到末端（p=n）\nCOLORS RGB\nTRACK RGB\nQUEUE RGR\nSHOOT 0\nSHOOT 4\nQUIT"
  },
  {
    "id": "case06",
    "num": "06",
    "title": "撤销再战",
    "input": "# case06 撤销后再消除：先插入（不消除）→ UNDO → 再插入形成三连\nCOLORS RB\nTRACK RRB\nQUEUE RBR\nSHOOT 3\nUNDO\nSHOOT 2\nQUIT"
  },
  {
    "id": "case07",
    "num": "07",
    "title": "连锁回滚",
    "input": "# case07 撤销多级连锁：连锁归空 WIN 后 UNDO 完整回滚\nCOLORS RB\nTRACK RRBBRR\nQUEUE B\nSHOOT 4\nUNDO\nQUIT"
  },
  {
    "id": "case08",
    "num": "08",
    "title": "炸弹开路",
    "input": "# case08 炸弹球 *：以炸弹为中心消除 R*G（右侧优先、至多 3 球）\nCOLORS RGB\nTRACK RGB\nQUEUE *R\nSHOOT 1\nQUIT"
  },
  {
    "id": "case09",
    "num": "09",
    "title": "变色挑战",
    "input": "# case09 变色球 ?：非法颜色报错（零副作用）→ 定色消除 → 普通球误带颜色报错 → 队列耗尽判负\nCOLORS RB\nTRACK RRB\nQUEUE ?R\nSHOOT 2 X\nSHOOT 2 R\nSHOOT 1 R\nSHOOT 1"
  },
  {
    "id": "case10",
    "num": "10",
    "title": "交换顺序",
    "input": "# case10 SWAP：交换队首两球后发射消除；剩余不足两球时报错\nCOLORS RB\nTRACK RRB\nQUEUE BR\nSWAP\nSHOOT 2\nSWAP\nQUIT"
  },
  {
    "id": "case11",
    "num": "11",
    "title": "提示与无解",
    "input": "# case11 HINT：存在最优位（0）与无可消除位（-1）\nCOLORS RB\nTRACK RRB\nQUEUE RB\nHINT\nSHOOT 2\nHINT\nQUIT"
  },
  {
    "id": "case12",
    "num": "12",
    "title": "倒退拼图",
    "input": "# case12 REVERSE：反转闭区间 [2,5] 拼出四连消除\nCOLORS RB\nTRACK RRBBRR\nQUEUE R\nREVERSE 2 5\nQUIT"
  },
  {
    "id": "case13",
    "num": "13",
    "title": "步数上限",
    "input": "# case13 LIMIT 判负：步数达到上限（即使队列未耗尽）\nCOLORS RB\nTRACK RB\nQUEUE RRR\nLIMIT 2\nSHOOT 0\nSHOOT 0"
  },
  {
    "id": "case14",
    "num": "14",
    "title": "队列耗尽",
    "input": "# case14 队列耗尽判负：轨道仍有球且无待发射球\nCOLORS RB\nTRACK RB\nQUEUE R\nSHOOT 0"
  },
  {
    "id": "case15",
    "num": "15",
    "title": "最后一球",
    "input": "# case15 胜利终局：最后一球补齐三连，轨道清空\nCOLORS RB\nTRACK RR\nQUEUE R\nSHOOT 2"
  },
  {
    "id": "case16",
    "num": "16",
    "title": "错误示范",
    "input": "# case16 错误输入：越界发射 + 空撤销栈（均不改状态、不计步）\nCOLORS RB\nTRACK RB\nQUEUE R\nSHOOT 99\nUNDO\nHINT\nQUIT"
  }
];
})(typeof globalThis !== "undefined" ? globalThis : this);
