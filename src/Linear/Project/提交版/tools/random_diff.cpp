/*
 * 祖玛游戏 · core 随机差分测试（阶段 6 补强，P1）
 *
 * 参考引擎：reference_game.hpp 用字符串操作与全量快照独立实现规则，
 * 每次比较合法性、轨道/队列/分数/步数/状态、完整消除事件与提示。
 * 原来的 U2/零副作用/撤销/重放断言继续保留。
 *
 * 背景：阶段 1 曾用临时程序做过 10^5 局随机差分，抓到 REVERSE 双拼缝
 * 的悬垂指针缺陷；但该程序没有入库，报告里的“10^5 差分”因此缺少可
 * 复现证据。本工具把它固化为仓库内的正式测试。
 *
 * 每局流程：
 *   1. 随机生成颜色集合、初始轨道、待发射队列（约 10% 特殊球）；
 *   2. 执行 0~6 步随机 SHOOT / SWAP / REVERSE（含少量非法操作）；
 *   3. 对每一步断言：
 *        - 合法操作：moves +1、分数单调不减、分数增量 = 事件 points 之和、
 *          每轮 points ∈ [0, kScoreCap]、轨道不存在 ≥3 同色段（U2）；
 *        - 非法操作：抛出异常且轨道/队列/分数/步数/状态完全不变；
 *   4. 全部撤销，断言精确恢复初始局面（轨道、队列、分数、步数、状态）；
 *   5. 用相同操作序列在新 Game 上重放，断言终态与首次一致（确定性）。
 *
 * 用法：
 *   ./tools/random_diff [-n 局数] [-s 种子]
 *   默认 100000 局、种子 20251020；失败时打印首个不一致的局号并退出 1。
 *
 * 构建/运行：make difftest（可用 make difftest DIFF_N=1000 缩短）。
 */
#include "core/game.hpp"
#include "reference_game.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <random>
#include <string>
#include <vector>

namespace {

struct Snapshot {
    std::string track;
    std::string queue;
    std::int64_t score = 0;
    int moves = 0;
    int status = 0;
};

Snapshot snapshot(const zuma::Game &g)
{
    Snapshot s;
    s.track = g.trackString();
    s.queue = g.queueString();
    s.score = g.score();
    s.moves = g.moves();
    s.status = static_cast<int>(g.status());
    return s;
}

bool sameReference(const Snapshot &a, const zuma_test::ReferenceGame &ref)
{
    const auto &b = ref.state;
    return a.track == b.track && a.queue == b.queue && a.score == b.score
        && a.moves == b.moves && a.status == b.status;
}

bool same(const Snapshot &a, const Snapshot &b)
{
    return a.track == b.track && a.queue == b.queue && a.score == b.score
           && a.moves == b.moves && a.status == b.status;
}

/* 不变量 U2：轨道上不得存在 ≥3 连同色段（特殊球符号不会出现在轨道）。 */
bool hasTriple(const std::string &track)
{
    int run = 1;
    for (std::size_t i = 1; i < track.size(); ++i) {
        run = (track[i] == track[i - 1]) ? run + 1 : 1;
        if (run >= 3)
            return true;
    }
    return false;
}

struct Op {
    enum Kind { kShoot, kSwap, kReverse } kind;
    int a = 0;      // SHOOT: pos；REVERSE: i
    int b = 0;      // REVERSE: j
    char color = 0; // SHOOT 变色球定色（0 = 未指定）
};

std::string randomBalls(std::mt19937_64 &rng, const std::string &colors,
                        int len, bool allowSpecial)
{
    std::string s;
    for (int i = 0; i < len; ++i) {
        if (allowSpecial && rng() % 10 == 0)
            s.push_back((rng() % 2 == 0) ? zuma::kBomb : zuma::kChameleon);
        else
            s.push_back(colors[rng() % colors.size()]);
    }
    return s;
}

struct Failure {
    int game = -1;
    std::string what;
};

class Runner {
public:
    Runner(std::uint64_t seed, int games)
        : rng_(seed), games_(games), seed_(seed)
    {
    }

    bool run()
    {
        for (int i = 0; i < games_; ++i) {
            if (!runGame(i)) {
                std::printf("FAIL 第 %d 局：%s\n", failure_.game,
                            failure_.what.c_str());
                return false;
            }
        }
        std::printf("ALL RANDOM DIFF TESTS PASSED (%d 局，seed=%llu)\n",
                    games_, static_cast<unsigned long long>(seed_));
        return true;
    }

private:
    bool fail(int game, const std::string &what)
    {
        failure_.game = game;
        failure_.what = what + "\n" + context_;
        return false;
    }

    bool runGame(int gameIndex)
    {
        // 1. 生成初始局面：2~3 种颜色、3~10 球轨道、2~6 球队列。
        const char *palette[] = {"RB", "RGB", "RGBY"};
        const std::string colors = palette[rng_() % 3];
        const int trackLen = 3 + static_cast<int>(rng_() % 8);
        const int queueLen = 2 + static_cast<int>(rng_() % 5);

        std::string track;
        std::string queue;
        for (int attempt = 0; attempt < 100; ++attempt) {
            track = randomBalls(rng_, colors, trackLen, false);
            queue = randomBalls(rng_, colors, queueLen, true);
            zuma::Game probe(colors, track, queue, 0);
            // 装载 settle 可能把轨道清空（判胜）；这种局面没有可操作空间，
            // 重新生成，保证初始状态为 PLAYING。
            if (probe.status() == zuma::Game::kPlaying)
                break;
        }

        const int limit = rng_() % 4 == 0 ? 1 + static_cast<int>(rng_() % 6) : 0;
        context_ = "COLORS " + colors + "\nTRACK " + track + "\nQUEUE " + queue
            + (limit > 0 ? "\nLIMIT " + std::to_string(limit) : "") + "\n";
        zuma::Game g(colors, track, queue, limit);
        zuma_test::ReferenceGame ref(colors, track, queue, limit);
        if (!sameReference(snapshot(g), ref))
            return fail(gameIndex, "初始化与独立参考实现不一致");
        if (g.status() != zuma::Game::kPlaying)
            return fail(gameIndex, "初始局面未处于 PLAYING");
        const Snapshot initial = snapshot(g);
        if (hasTriple(g.trackString()))
            return fail(gameIndex, "初始轨道违反 U2");

        // 2. 随机操作。
        std::vector<Op> applied;
        const int steps = static_cast<int>(rng_() % 7);
        for (int step = 0; step < steps; ++step) {
            const int roll = static_cast<int>(rng_() % 100);
            Op op;
            if (roll < 60) {
                op.kind = Op::kShoot;
                const std::size_t n = g.track().size();
                op.a = static_cast<int>(rng_() % (n + 1));
                if (!g.queue().empty()
                    && g.queue().front() == zuma::kChameleon) {
                    // 90% 合法定色；10% 故意不指定（应抛异常且零副作用）
                    if (rng_() % 10 != 0)
                        op.color = colors[rng_() % colors.size()];
                } else if (!g.queue().empty() && rng_() % 20 == 0) {
                    // 5% 给普通球带颜色（非法，应抛异常且零副作用）
                    op.color = colors[rng_() % colors.size()];
                }
            } else if (roll < 80) {
                op.kind = Op::kSwap;
            } else {
                op.kind = Op::kReverse;
                const std::size_t n = g.track().size();
                if (n >= 2 && rng_() % 10 != 0) {
                    // 90% 合法区间
                    op.a = static_cast<int>(rng_() % n);
                    op.b = op.a + static_cast<int>(
                                      rng_() % (n - static_cast<std::size_t>(op.a)));
                } else {
                    // 10% 故意非法的区间（含空轨道 / j<i）
                    op.a = static_cast<int>(rng_() % 5);
                    op.b = op.a - 1;
                }
            }

            if (g.hint() != ref.hint())
                return fail(gameIndex, "HINT 与独立参考实现不一致");
            context_ += op.kind == Op::kShoot ? "SHOOT " + std::to_string(op.a)
                + (op.color ? " " + std::string(1, op.color) : "") + "\n" :
                op.kind == Op::kSwap ? "SWAP\n" :
                "REVERSE " + std::to_string(op.a) + " " + std::to_string(op.b) + "\n";
            const Snapshot before = snapshot(g);
            std::vector<zuma::Game::ElimEvent> events;
            bool ok = true;
            try {
                switch (op.kind) {
                case Op::kShoot:
                    events = g.shoot(op.a, op.color);
                    break;
                case Op::kSwap:
                    g.swap();
                    break;
                case Op::kReverse:
                    events = g.reverse(op.a, op.b);
                    break;
                }
            } catch (const std::exception &) {
                ok = false;
            }

            bool refOk = true;
            std::vector<zuma_test::ReferenceGame::Event> expectedEvents;
            try {
                switch (op.kind) {
                case Op::kShoot: expectedEvents = ref.shoot(op.a, op.color); break;
                case Op::kSwap: ref.swap(); break;
                case Op::kReverse: expectedEvents = ref.reverse(op.a, op.b); break;
                }
            } catch (const std::exception &) { refOk = false; }
            if (ok != refOk || !sameReference(snapshot(g), ref)
                || g.undoAvailable() != ref.undoAvailable())
                return fail(gameIndex, "操作合法性/状态与独立参考实现不一致");
            if (events.size() != expectedEvents.size())
                return fail(gameIndex, "消除事件数量与参考实现不一致");
            for (std::size_t e = 0; e < events.size(); ++e) {
                const auto &a = events[e];
                const auto &b = expectedEvents[e];
                if (a.round != b.round || a.left != b.left || a.count != b.count
                    || a.points != b.points || a.balls != b.balls || a.trackAfter != b.trackAfter)
                    return fail(gameIndex, "消除事件字段与参考实现不一致");
            }

            if (!ok) {
                // 非法操作必须零副作用（变色球吞队首球缺陷的回归点）
                if (!same(before, snapshot(g)))
                    return fail(gameIndex, "非法操作改变了游戏状态");
                continue;
            }

            applied.push_back(op);

            const Snapshot after = snapshot(g);
            if (after.moves != before.moves + 1)
                return fail(gameIndex, "合法操作未使 steps +1");
            if (after.score < before.score)
                return fail(gameIndex, "分数下降");
            if (after.score > zuma::Game::kScoreCap)
                return fail(gameIndex, "分数超过 kScoreCap");
            if (hasTriple(after.track))
                return fail(gameIndex, "操作后轨道违反 U2");

            std::int64_t sum = 0;
            for (const auto &ev : events) {
                if (ev.points < 0)
                    return fail(gameIndex, "事件 points 为负");
                if (ev.points > zuma::Game::kScoreCap)
                    return fail(gameIndex, "事件 points 超过 kScoreCap");
                sum = ev.points >= zuma::Game::kScoreCap - sum
                    ? zuma::Game::kScoreCap : sum + ev.points;
            }
            const auto expectedScore = sum >= zuma::Game::kScoreCap - before.score
                ? zuma::Game::kScoreCap : before.score + sum;
            if (expectedScore != after.score)
                return fail(gameIndex, "分数增量不等于事件 points 之和");
        }

        const Snapshot finalState = snapshot(g);

        // 3. 全部撤销，恢复初始局面。
        int undone = 0;
        while (g.undoAvailable()) {
            if (!g.undo())
                return fail(gameIndex, "undoAvailable 为真但 undo 返回 false");
            if (!ref.undo() || !sameReference(snapshot(g), ref))
                return fail(gameIndex, "逐步撤销与参考实现不一致");
            if (++undone > 1000)
                return fail(gameIndex, "撤销次数异常");
            if (hasTriple(g.trackString()))
                return fail(gameIndex, "撤销后轨道违反 U2");
        }
        if (!same(initial, snapshot(g)))
            return fail(gameIndex, "撤销到底未恢复初始局面");

        // 4. 重放确定性：同一初始局面 + 同一操作序列应得到同一终态。
        zuma::Game replay(colors, track, queue, limit);
        for (const Op &op : applied) {
            switch (op.kind) {
            case Op::kShoot:
                (void)replay.shoot(op.a, op.color);
                break;
            case Op::kSwap:
                replay.swap();
                break;
            case Op::kReverse:
                (void)replay.reverse(op.a, op.b);
                break;
            }
        }
        if (!same(finalState, snapshot(replay)))
            return fail(gameIndex, "重放终态与首次不一致");

        return true;
    }

    std::mt19937_64 rng_;
    int games_ = 0;
    std::uint64_t seed_ = 0;
    Failure failure_;
    std::string context_;
};

void usage(const char *argv0)
{
    std::printf("用法：%s [-n 局数] [-s 种子]\n"
                "  -n  随机局数（默认 100000）\n"
                "  -s  随机种子（默认 20251020，固定种子保证可复现）\n"
                "  -h  显示本帮助\n",
                argv0);
}

} // namespace

int main(int argc, char **argv)
{
    int games = 100000;
    std::uint64_t seed = 20251020;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            games = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            seed = static_cast<std::uint64_t>(
                std::strtoull(argv[++i], nullptr, 10));
        } else if (std::strcmp(argv[i], "-h") == 0
                   || std::strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 2;
        }
    }
    if (games <= 0) {
        std::fprintf(stderr, "-n 必须为正整数\n");
        return 2;
    }

    Runner runner(seed, games);
    return runner.run() ? 0 : 1;
}