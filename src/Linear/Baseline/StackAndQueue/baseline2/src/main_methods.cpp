/*
 * Baseline 任务 2 · 小任务 2：两种"front/rear 到达数组尾部"解决方案对比。
 *
 * 方案 A（给定骨架 CircularQueue）：多分配一个空槽区分空/满（sentinel 槽）。
 * 方案 B（本目录 SizedQueue）：front 下标 + size 计数，rear = front + size，
 *                              不牺牲空槽（缓冲区恰好 capacity 个）。
 *
 * 本程序对两种方案跑同一操作序列，交叉校验行为一致，并给出性能统计。
 * 编译为 queue_methods。
 */
#include "sized_queue.hpp"
#include "../../src/queue/queue.hpp"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

static int gPassed = 0;
static int gFailed = 0;

static void expectInt(const std::string &name, long long actual, long long expected)
{
    if (actual == expected) gPassed++;
    else { gFailed++; std::cerr << "[FAIL] " << name << "：期望 " << expected << "，实际 " << actual << '\n'; }
}

/*
 * 在给定队列上重放一段操作，返回逐步输出字符串（队首→队尾遍历用倒队技巧）。
 * 两方案应产生完全一致的输出。
 */
template <typename Q>
static std::string replay(Q q)
{
    std::ostringstream out;
    // 固定序列：入队 1..5（装满），出队 2 个，再入队 6、7（跨下标回绕），最后全部出队
    for (int i = 1; i <= 5; ++i) q.push(i);
    out << q.front() << ' ';  // 1
    q.pop();
    out << q.front() << ' ';  // 2
    q.pop();
    for (int i = 6; i <= 7; ++i) q.push(i);  // 此时 3 个在队，再入 2 个 → 满
    while (!q.empty()) { out << q.front() << ' '; q.pop(); }
    out << '\n';
    return out.str();
}

template <typename Fn>
static double measureTimeMs(Fn &&fn)
{
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

static volatile long long g_sink = 0;

int main()
{
    // 两方案行为一致性（容量 5，恰好装 5 个，跨下标回绕）
    std::string a = replay(dsaac::CircularQueue<int>(5));
    std::string b = replay(dsaac::SizedQueue<int>(5));
    expectInt("两方案行为一致", a == b ? 1 : 0, 1);

    // 方案 B 满载：容量 3 可真正存满 3 个（不牺牲空槽）
    {
        dsaac::SizedQueue<int> q(3);
        q.push(1); q.push(2); q.push(3);
        expectInt("SizedQueue 容量 3 满载", q.full() ? 1 : 0, 1);
        bool threw = false;
        try { q.push(4); } catch (const std::overflow_error &) { threw = true; } catch (...) {}
        expectInt("SizedQueue 满后 push 上溢", threw ? 1 : 0, 1);
    }
    // 方案 A 同样容量 3 也可存 3 个（空槽是内部多分配的，不影响外部容量）
    {
        dsaac::CircularQueue<int> q(3);
        q.push(1); q.push(2); q.push(3);
        expectInt("CircularQueue 容量 3 满载", q.full() ? 1 : 0, 1);
    }
    std::cout << "# 两种解法对照：通过 " << gPassed << " 项，失败 " << gFailed << " 项\n";

    // 性能对比
    const long long sizes[] = {100000, 1000000};
    const int REPEAT = 3;
    for (long long n : sizes) {
        double tA = 0.0, tB = 0.0;
        for (int r = 0; r < REPEAT; ++r) {
            tA += measureTimeMs([&n]() {
                dsaac::CircularQueue<int> q(static_cast<std::size_t>(n));
                for (long long i = 0; i < n; ++i) q.push(static_cast<int>(i));
                long long s = 0;
                while (!q.empty()) { s += q.front(); q.pop(); }
                g_sink += s;
            });
            tB += measureTimeMs([&n]() {
                dsaac::SizedQueue<int> q(static_cast<std::size_t>(n));
                for (long long i = 0; i < n; ++i) q.push(static_cast<int>(i));
                long long s = 0;
                while (!q.empty()) { s += q.front(); q.pop(); }
                g_sink += s;
            });
        }
        tA /= REPEAT; tB /= REPEAT;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "PERF circular_queue n=" << n << " ops=" << n * 2 << " time_ms=" << tA << '\n';
        std::cout << "PERF sized_queue n=" << n << " ops=" << n * 2 << " time_ms=" << tB << '\n';
    }

    return gFailed > 0 ? 1 : 0;
}