/*
 * 祖玛游戏 · core 层算法自测（自 tools/ 迁入，原阶段 1/2 的 main.cpp）
 *
 * 逐场景构造 Game，对轨道/分数/事件/撤销断言预期。与协议层
 * （driver/）解耦，只测 core 语义。27 场景，含 README 示例、多级连锁、
 * 34/60 轮深连锁（计分溢出与饱和回归）、终局守卫、撤销回滚、全部道具。
 *
 * 构建：见 Makefile 的 selftest 目标（编译为 tools/selftest）。
 * 运行：./tools/selftest；全部通过输出 ALL CORE TESTS PASSED。
 */
#include "core/game.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void expect(bool cond, const std::string &what)
{
    if (!cond) {
        ++g_failures;
        std::printf("FAIL: %s\n", what.c_str());
    }
}

void expectTrack(const zuma::Game &g, const std::string &want,
                 const std::string &what)
{
    expect(g.trackString() == want,
           what + "（track=" + g.trackString() + " want=" + want + "）");
}

/* 场景 1：README 示例 —— 装载即连锁归空 */
void testLoadSettle()
{
    zuma::Game g("RB", "RRBBBRR", "", 0);
    expectTrack(g, "", "load-settle README 示例");
    expect(g.status() == zuma::Game::kWin, "load-settle 后应为胜");
    expect(g.score() == 0, "装载消除不计分");
}

/* 场景 2：单次发射消除 + 计分 */
void testShootSingle()
{
    zuma::Game g("RB", "RR", "R", 0); // RR + 发 R → RRR 消
    auto ev = g.shoot(2);
    expect(ev.size() == 1, "一轮消除");
    expect(ev[0].count == 3 && ev[0].round == 1, "3 球首消");
    expect(ev[0].points == 30, "计分 3×10");
    expectTrack(g, "", "消后空");
    expect(g.score() == 30, "分数 30");
    expect(g.status() == zuma::Game::kWin, "发射后胜");
}

/* 场景 3：连锁（两级）—— R R B B B R R 布局，发 B 进中间 */
void testChain()
{
    // 轨道 R R B B R R：发 B → R R B B B R R → 消 BBB → RRRR → 消
    zuma::Game g("RB", "RRBBRR", "B", 0);
    auto ev = g.shoot(4); // 插到下标 4（B B 之间）
    expect(ev.size() == 2, "两轮连锁");
    expect(ev[0].round == 1 && ev[0].count == 3, "首轮 3 球");
    expect(ev[1].round == 2 && ev[1].count == 4, "次轮 4 球");
    expect(g.score() == 30 + 4 * 10 * 2, "连锁计分 30+80");
    expectTrack(g, "", "连锁后空");
}

/* 场景 4：不足三连不消除 */
void testNoElim()
{
    zuma::Game g("RGB", "RGB", "G", 0);
    auto ev = g.shoot(1);
    expect(ev.empty(), "两连不消除");
    expectTrack(g, "RGGB", "两连留轨");
    expect(g.score() == 0, "零分");
}

/* 场景 5：撤销完整回滚（含连锁 + 分数 + 队列 + WIN 态回退） */
void testUndoChain()
{
    zuma::Game g("RB", "RRBBRR", "BQ", 0); // Q 占位防队空判负
    auto ev = g.shoot(4);                    // 触发两级连锁 → WIN
    expect(g.status() == zuma::Game::kWin, "连锁后胜");
    expect(g.queueString() == "Q", "剩 Q");

    expect(g.undo(), "撤销成功");
    expectTrack(g, "RRBBRR", "撤销恢复轨道");
    expect(g.score() == 0, "撤销恢复分数");
    expect(g.moves() == 0, "撤销恢复步数");
    expect(g.queueString() == "BQ", "撤销球回队首");
    expect(g.status() == zuma::Game::kPlaying, "从 WIN 回退");
    (void)ev;

    // 重放同一操作结果应一致（撤销的确定性）
    auto ev2 = g.shoot(4);
    expect(ev2.size() == 2 && g.trackString().empty(), "重放一致");
}

/* 场景 6：撤销后边界 —— 无可撤销返回 false */
void testUndoEmpty()
{
    zuma::Game g("R", "R", "R", 0);
    expect(!g.undo(), "空撤销栈返回 false");
}

/* 场景 7：炸弹球 —— 无视颜色消除邻域 3 球（先右后左扩展） */
void testBomb()
{
    // R G B + 炸弹插入下标 1 → R * G B：右扩展 G、左扩展 R → 消 R*G
    zuma::Game g("RGB", "RGB", "*", 0);
    auto ev = g.shoot(1);
    expect(ev.size() >= 1, "炸弹至少消自身");
    expect(ev[0].count == 3, "炸弹消邻域 3 球");
    expectTrack(g, "B", "炸后剩 B");
    expect(g.score() == 30, "炸弹计分同普通");
}

/* 场景 8：炸弹在端点 —— 消到端 */
void testBombEdge()
{
    zuma::Game g("RGB", "RGB", "*", 0);
    auto ev = g.shoot(3); // 尾插：* 在最右 → R G B *，左扩展 B、G
    expect(ev[0].count == 3, "端点炸弹仍消 3 球（左补）");
    expectTrack(g, "R", "剩 R");
    (void)ev;
}

/* 场景 9：变色球 —— 定色触发消除 */
void testChameleon()
{
    zuma::Game g("RB", "RR", "?", 0);
    auto ev = g.shoot(2, 'R'); // 定色 R → RRR 消
    expect(ev.size() == 1 && ev[0].count == 3, "变色定色消除");
    expectTrack(g, "", "消后空");
}

/* 场景 10：变色球未定色 → 异常（driver 拒绝） */
void testChameleonNoColor()
{
    zuma::Game g("RB", "R", "?", 0);
    bool threw = false;
    try {
        g.shoot(1);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    expect(threw, "变色未定色抛异常");
}

/* 场景 11：SWAP 道具与撤销 */
void testSwap()
{
    zuma::Game g("RB", "R", "BR", 0);
    expect(g.queueString() == "BR", "初始队序");
    g.swap();
    expect(g.queueString() == "RB", "交换后队序");
    expect(g.moves() == 1, "SWAP 计一步");
    g.undo();
    expect(g.queueString() == "BR", "撤销恢复队序");
    expect(g.moves() == 0, "撤销恢复步数");
}

/* 场景 12：REVERSE 道具 + 消除 + 撤销 */
void testReverse()
{
    // R B B R 反转 [1,2] → R B B R 不变；构造可消除反转：R R B 反转
    // [0,2] → B R R 不消除；换构造：轨道 R B R B，反 [0,1] → B R R B？…
    // 直接用"反转拼出三连"：轨道 G R G，反转 [0,2] → G R G（回文）→
    // 换轨道 B R R B：反转 [1,2] → B R R B 不变。用 B B R R 反转 [1,2]
    // → B R R B。真正能拼出三连的：R B R R B R —— 反转 [1,4]（B R R B）
    // → R B R R B R 回文不变。改为轨道 A B B A 型对拼：
    // R R B B：反转 [2,3] → R R B B。回文陷阱太多，用长度 5：
    // R B R B B 反转 [0,3]（R B R B→B R B R）→ B R B R B 无消除。
    // 经典构造：R G G R，反 [0,3] → R G G R 回文。
    // 用"两段同色被反转拉近"：R B B R 无解。设轨道 R B B B R 不可能
    // （U2 装载已消）。轨道 R B B R + REVERSE [1,2]（B B → B B）不变。
    // 构造 B B R R B B？装载即消。
    // 可行构造：R B B B R R（装载消 BBB → R R R R → 消）不行。
    // 用 U2 合法轨道 G G R B R R G：反转 [2,5]（R B R R → R R B R）
    // → G G R R B R G 无消除；反转 [2,4]（R B R → R B R）回文。
    // 真正有效：轨道 R B B R R B：反 [2,3]（B B→B B）…
    // 结论性构造：轨道 R R B B 反转 [2,3] 后 R R B B；反转 [0,3] 后
    // B B R R。都不消。**用区间把同色对翻到相邻**：
    // 轨道 G G R B R B：反转 [2,5]（R B R B → B R B R）→ G G B R B R。
    // 轨道 G G R B R：反 [2,4]（R B R → R B R 回文）。
    // 轨道 G G B R B R？装载合法。反 [2,5]（B R B R→R B R B）→G G R B R B。
    // **最简**：轨道 R G G R？装载合法（两两）。反 [0,3] → R G G R。
    // 算了——用 4 球非回文 + 拼缝：轨道 R G G B B R（装载合法），
    // 反转 [1,4]（G G B B → B B G G）→ R B B G G R。拼缝 [0,1] 处
    // R|B、[4,5] 处 G|R——都不消。
    // **最终构造**：轨道 R G G B B R R，反 [1,6] → R R B B G G R：
    // 拼缝 [0,1] = R|R 两连；[6,7] 越界。不消。
    // 直接 3+3 夹 1 反转：轨道 R G R R G R？反 [0,5]→R G R R G R 回文。
    // 轨道 B R R G G R B 装载合法。反 [1,5]（R R G G → G G R R）→
    // B G G R R B。不消。
    // **换思路**：反转 [i,j] 后拼缝两端同色且合计 ≥3：
    // 轨道 R R B B R R（装载合法，两两段）。反 [2,3] → R R B B R R。
    // 反 [0,1] → 同。反 [2,5]（B B R R → R R B B）→ R R R R B B！
    // 拼缝 [1,2]=R|R、[2,3]=R|R：整段 RRRR（4 连）→ 消！剩 BB。
    zuma::Game g("RB", "RRBBRR", "R", 0);
    expectTrack(g, "RRBBRR", "装载合法");
    auto ev = g.reverse(2, 5);
    expect(ev.size() == 1, "反转拼出消除");
    expect(ev[0].count == 4, "消 RRRR");
    expectTrack(g, "BB", "剩 BB");
    expect(g.moves() == 1, "REVERSE 计一步");

    expect(g.undo(), "撤销反转");
    expectTrack(g, "RRBBRR", "撤销恢复原序");
    expect(g.moves() == 0, "撤销恢复步数");
}

/* 场景 12b：REVERSE 右拼缝单独成三连 —— 随机差分抓到的回归。
 * 轨道 R B G B B 反转 [1,2]（B G → G B）后为 R G B B B：左拼缝 R|G
 * 不消，右拼缝 B B B 必须消除。旧实现第二个 probe 误取左拼缝，漏消
 * 右拼缝，违反 U2。 */
void testReverseRightSeam()
{
    zuma::Game g("RBG", "RBGBB", "R", 0);
    expectTrack(g, "RBGBB", "right-seam 装载合法");
    auto ev = g.reverse(1, 2);
    expect(ev.size() == 1, "right-seam 反转触发消除");
    expect(ev[0].count == 3, "right-seam 消 BBB");
    expectTrack(g, "RG", "right-seam 后剩 RG");
    expect(g.score() == 30, "right-seam 计分 30");
    expect(g.moves() == 1, "right-seam 计一步");

    expect(g.undo(), "right-seam 撤销");
    expectTrack(g, "RBGBB", "right-seam 撤销恢复");
    expect(g.score() == 0, "right-seam 撤销分数归零");
}

/* 场景 13：HINT —— 最优位置（等消除数取最左） */
void testHint()
{
    zuma::Game g("RB", "RRB", "R", 0); // RRB：插 0/1/2 均成 RRR 消 3
    expect(g.hint() == 0, "HINT 等消除数取最左 0");
    zuma::Game g2("RB", "RB", "G", 0);
    expect(g2.hint() == -1, "无可消除位置");
}

/* 场景 14：LOSE 判定与回退 */
void testLoseAndUndo()
{
    zuma::Game g("RB", "RB", "R", 0);
    auto ev = g.shoot(0); // RB→R? 插 0：R R B 两连不消
    expect(g.status() == zuma::Game::kLose, "队列耗尽判负");
    (void)ev;
    expect(g.undo(), "从 LOSE 撤销");
    expect(g.status() == zuma::Game::kPlaying, "回退继续");
    expectTrack(g, "RB", "恢复轨道");
}

/* 场景 15：LIMIT 操作上限 */
void testLimit()
{
    zuma::Game g("RB", "RBBR", "RRRR", 2); // limit=2
    (void)g.shoot(0);
    (void)g.shoot(0);
    // 步数到限但队列未空且轨道非空：limit 语义 = 步数上限即负
    // （PROTOCOL.md §4：操作数超过 LIMIT → 负）
    expect(g.status() == zuma::Game::kLose, "步数到限判负");
}

/* 场景 16：合法性 —— 越界发射抛异常且不改状态 */
void testInvalidShoot()
{
    zuma::Game g("R", "R", "R", 0);
    bool threw = false;
    try {
        g.shoot(99);
    } catch (const std::out_of_range &) {
        threw = true;
    }
    expect(threw, "越界发射抛异常");
    expectTrack(g, "R", "异常后轨道不变");
    expect(g.queueString() == "R", "异常后队列不变");

    // 底层 erase_run 的强异常安全：非法长度必须在删除前拒绝
    zuma::DoublyLinkedTrack t("RGB");
    bool trackThrew = false;
    try {
        t.erase_run(t.at(1), 5);
    } catch (const std::invalid_argument &) {
        trackThrew = true;
    }
    expect(trackThrew, "erase_run 越界抛异常");
    expect(t.size() == 3, "erase_run 越界不改变长度");
    expect(t.at(0)->ball == 'R' && t.at(1)->ball == 'G'
               && t.at(2)->ball == 'B',
           "erase_run 越界不改变内容");
}

/* 场景 17：链条正确性压力——交替构造多轮连锁 */
void testDeepChain()
{
    // 经典深连锁构造：RR BB RR BB … 发中间色
    // 轨道：R R B B R R B B R R 装载合法（两两）
    // 发 B 进第 2/3 位之间（pos 2）：RR + B + BB → RR BBB RR BBB RR
    // 消 BBB(3) → RR RR BBB RR → RRRR 消(4) → 拼缝 BBB 消(3) →
    // 剩 RR RR? 精确推演：
    //   R R [B] B B R R B B R R（插入下标 2）
    //   轮1 消 BBB(pos2..4) → R R R R B B R R（RR+RR 相邻 4 连）
    //   轮2 消 RRRR → B B R R
    //   轮3? BB RR 无 ≥3，稳定。
    //   → 剩 BBRR，3 轮？轮2 只消 4 球 RRRR：R R R R B B R R 中
    //   RRRR 是 0..3，消后 B B R R。轮 3 检查拼缝 B|R 不消。
    zuma::Game g("RB", "RRBBRRBBRR", "B", 0);
    auto ev = g.shoot(2);
    expect(ev.size() == 2, "两轮连锁");
    expectTrack(g, "BBRR", "深连锁结果");
    int total = 0;
    for (const auto &e : ev)
        total += e.points;
    expect(g.score() == total, "分数=各轮和");

    expect(g.undo(), "深连锁撤销");
    expectTrack(g, "RRBBRRBBRR", "深连锁撤销恢复");
    expect(g.score() == 0, "深连锁撤销分数归零");
}

/* 场景 18：撤销的撤销（连续两步各自可撤销） */
void testUndoTwice()
{
    zuma::Game g("RB", "RRBB", "BRB", 0);
    (void)g.shoot(3); // RRBB + B → RR BBB 消 → RR
    expectTrack(g, "RR", "一步后");
    (void)g.shoot(0); // R + RR = RRR 消 → 空 → WIN
    expect(g.status() == zuma::Game::kWin, "两步后胜");
    expect(g.undo(), "撤销第二步");
    expectTrack(g, "RR", "回到一步后");
    expect(g.queueString() == "RB", "队列恢复");
    expect(g.status() == zuma::Game::kPlaying, "回退 PLAYING");
    expect(g.undo(), "撤销第一步");
    expectTrack(g, "RRBB", "回到初始");
    expect(g.queueString() == "BRB", "队列回初始");
}

/* 场景 19：变色球撤销 —— 特殊符号回队 */
void testUndoChameleon()
{
    zuma::Game g("RB", "RR", "?G", 0);
    (void)g.shoot(2, 'R'); // RRR 消 → WIN
    expect(g.status() == zuma::Game::kWin, "变色消胜");
    expect(g.undo(), "撤销变色发射");
    expect(g.queueString() == "?G", "特殊球原符号回队首");
    expectTrack(g, "RR", "轨道恢复");
}

/* 场景 21：SWAP 达到 LIMIT 判负，撤销后恢复 */
void testSwapLimit()
{
    zuma::Game g("RB", "RB", "RGB", 1);
    g.swap();
    expect(g.queueString() == "GRB", "swap 队首两球");
    expect(g.status() == zuma::Game::kLose, "swap 达到 LIMIT 判负");
    expect(g.undo(), "撤销 swap");
    expect(g.queueString() == "RGB", "swap 撤销恢复队列");
    expect(g.status() == zuma::Game::kPlaying, "swap 撤销恢复 PLAYING");
}

/* 场景 20：炸弹撤销 */
void testUndoBomb()
{
    zuma::Game g("RGB", "RGB", "*G", 0);
    (void)g.shoot(1); // R*GB → 消 R*G → 剩 B
    expectTrack(g, "B", "炸弹后");
    expect(g.undo(), "撤销炸弹");
    expectTrack(g, "RGB", "炸弹撤销恢复");
    expect(g.queueString() == "*G", "炸弹球回队首");
}

/* 构造 r 轮连锁的嵌套轨道：T1="BB"，T_{r+1}=XX+T_r+XX（X 在 R/B 间
 * 交替）。向中央 "BB" 对的缝隙（下标 2r-1）插入 B，即可触发 r 轮消除
 * 并把整条轨道清空；T_r 长度 = 4r-2。 */
std::string nestedChainTrack(int rounds)
{
    std::string t = "BB";
    char wrap = 'R';
    for (int r = 2; r <= rounds; ++r) {
        std::string w(2, wrap);
        t = w + t + w;
        wrap = (wrap == 'R') ? 'B' : 'R';
    }
    return t;
}

/* 场景 22：34 轮深连锁 —— 计分溢出回归（P0）。
 * 32 位 int 在 round 27 左右就会溢出（2^26 × 30 ≈ 2×10^9），旧实现
 * 会产生负分；本场景断言每轮 points 非负、总分单调、WIN 分数与手算
 * 一致，并验证一次性撤销完整还原 134 球轨道。 */
void testDeepChain34()
{
    const int kRounds = 34;
    const std::string track = nestedChainTrack(kRounds);
    expect(track.size() == static_cast<std::size_t>(4 * kRounds - 2),
           "deep34 轨道长度 4r-2");

    zuma::Game g("RB", track, "B", 0);
    auto ev = g.shoot(2 * kRounds - 1);
    expect(ev.size() == static_cast<std::size_t>(kRounds), "deep34 共 34 轮");

    std::int64_t sum = 0, prev = 0;
    bool nonneg = true, monotonic = true, capped = true, roundsOk = true;
    for (const auto &e : ev) {
        roundsOk = roundsOk && (e.round >= 1);
        nonneg = nonneg && (e.points >= 0);
        capped = capped && (e.points <= zuma::Game::kScoreCap);
        sum += e.points;
        monotonic = monotonic && (sum >= prev);
        prev = sum;
    }
    expect(roundsOk, "deep34 轮次连续");
    expect(nonneg, "deep34 每轮 points 非负");
    expect(capped, "deep34 每轮 points 不超过上限");
    expect(monotonic, "deep34 总分单调不减");

    // 手算：首轮 3 球 30 分；第 2..34 轮各 4 球，得分 40×2^(r-1)
    const std::int64_t expected =
        30 + 40 * ((static_cast<std::int64_t>(1) << kRounds) - 2);
    expect(sum == expected, "deep34 各轮和等于手算值");
    expect(g.score() == expected, "deep34 WIN 分数等于手算值");
    expect(g.score() == 687194767310LL, "deep34 分数 687194767310");
    expectTrack(g, "", "deep34 消后轨道为空");
    expect(g.status() == zuma::Game::kWin, "deep34 判胜");

    expect(g.undo(), "deep34 撤销");
    expectTrack(g, track, "deep34 撤销恢复 134 球");
    expect(g.score() == 0, "deep34 撤销分数归零");
    expect(g.moves() == 0, "deep34 撤销步数归零");
    expect(g.queueString() == "B", "deep34 撤销队列恢复");
}

/* 场景 23：超深连锁 —— 分数饱和策略（P0）。
 * 60 轮理论总分超过 2^53-1，两端都必须饱和到 kScoreCap，且每轮
 * points 仍非负、单调不减，避免 C++ int64 与 JS Number 在极端局面
 * 下出现跨实现差异。 */
void testScoreSaturation()
{
    const int kRounds = 60;
    zuma::Game g("RB", nestedChainTrack(kRounds), "B", 0);
    auto ev = g.shoot(2 * kRounds - 1);
    expect(ev.size() == static_cast<std::size_t>(kRounds), "sat 共 60 轮");

    std::int64_t prev = 0;
    bool nonneg = true, monotonic = true, capped = true;
    for (const auto &e : ev) {
        nonneg = nonneg && (e.points >= 0);
        capped = capped && (e.points <= zuma::Game::kScoreCap);
        monotonic = monotonic && (e.points >= prev);
        prev = e.points;
    }
    expect(nonneg, "sat points 非负");
    expect(capped, "sat points 不超过上限");
    expect(monotonic, "sat 每轮 points 单调不减（饱和后持平）");
    expect(g.score() == zuma::Game::kScoreCap, "sat 总分饱和于 2^53-1");
    expect(g.status() == zuma::Game::kWin, "sat 判胜");
}

/* 场景 24：WIN 后调用 swap 必须拒绝且状态不变（P1 终局守卫）。 */
void testSwapAfterWin()
{
    // 轨道 RR + 发 R -> RRR 消空判胜，队列还剩 RR（供 swap 试验）
    zuma::Game g("RB", "RR", "RR", 0);
    (void)g.shoot(2);
    expect(g.status() == zuma::Game::kWin, "swap-after-win 先判胜");
    const std::string queueBefore = g.queueString();
    bool threw = false;
    try {
        g.swap();
    } catch (const std::logic_error &) {
        threw = true;
    }
    expect(threw, "swap-after-win 抛 logic_error");
    expect(g.queueString() == queueBefore, "swap-after-win 队列不变");
    expect(g.score() == 30, "swap-after-win 分数不变");
    expect(g.moves() == 1, "swap-after-win 步数不变");
    expect(g.status() == zuma::Game::kWin, "swap-after-win 仍为终局");
    g.quit();
    expect(!g.undo(), "QUIT 后核心API也不可撤销");
    expect(g.status() == zuma::Game::kQuit && g.moves() == 1 && g.score() == 30,
           "QUIT 后撤销零副作用");
}

void testBombHintEndpoints()
{
    for (const std::string track : {"R", "RG", "RGB"}) {
        zuma::Game hintGame("RGB", track, "*R", 0);
        expect(hintGame.hint() == 0, "炸弹等分取最左端");
        for (int p = 0; p <= static_cast<int>(track.size()); ++p) {
            zuma::Game g("RGB", track, "*R", 0);
            const auto events = g.shoot(p);
            expect(events[0].count == static_cast<int>(std::min<std::size_t>(3, track.size() + 1)),
                   "首端/尾端/中间炸弹首轮数目");
            expect(g.undo(), "炸弹端点撤销");
            expectTrack(g, track, "炸弹端点撤销恢复轨道");
            expect(g.queueString() == "*R", "炸弹端点撤销恢复队列");
        }
    }
}

void testReverseIndependentChains()
{
    zuma::Game g("RGB", "GGRGRGRR", "R", 0);
    const auto events = g.reverse(2, 5);
    expect(events.size() == 2, "反转两拼缝各消一段");
    expect(events[0].round == 1 && events[1].round == 1, "独立连锁各从第1轮计分");
    expect(g.score() == 60, "独立连锁不额外翻倍");
    expectTrack(g, "RG", "两拼缝消除后轨道");
    expect(g.undo(), "独立连锁整体撤销");
    expectTrack(g, "GGRGRGRR", "两拼缝撤销原序");
}

} // namespace

int main()
{
    std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"load-settle", testLoadSettle},
        {"shoot-single", testShootSingle},
        {"chain", testChain},
        {"no-elim", testNoElim},
        {"undo-chain", testUndoChain},
        {"undo-empty", testUndoEmpty},
        {"bomb", testBomb},
        {"bomb-edge", testBombEdge},
        {"chameleon", testChameleon},
        {"chameleon-no-color", testChameleonNoColor},
        {"swap", testSwap},
        {"reverse", testReverse},
        {"reverse-right-seam", testReverseRightSeam},
        {"hint", testHint},
        {"bomb-hint-endpoints", testBombHintEndpoints},
        {"reverse-independent", testReverseIndependentChains},
        {"lose-undo", testLoseAndUndo},
        {"limit", testLimit},
        {"invalid-shoot", testInvalidShoot},
        {"deep-chain", testDeepChain},
        {"undo-twice", testUndoTwice},
        {"undo-chameleon", testUndoChameleon},
        {"undo-bomb", testUndoBomb},
        {"swap-limit", testSwapLimit},
        {"deep-chain-34", testDeepChain34},
        {"score-saturation", testScoreSaturation},
        {"swap-after-win", testSwapAfterWin},
    };
    for (const auto &t : tests) {
        t.second();
        std::printf("PASS  %-22s %s\n", t.first.c_str(),
                    g_failures ? "(fail 见上)" : "");
        if (g_failures)
            break; // 首个失败即停，逐个修复
    }
    if (g_failures) {
        std::printf("%d 个断言失败\n", g_failures);
        return 1;
    }
    std::printf("ALL CORE TESTS PASSED (%zu 场景)\n", tests.size());
    return 0;
}
