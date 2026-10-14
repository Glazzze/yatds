/*
 * Baseline 任务 2 · 小任务 4：扩展队列五功能自测 + 与 std::queue 性能对比。
 * 编译为 queue_extend。
 */
#include "extended_queue.hpp"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <string>

static int gPassed = 0;
static int gFailed = 0;

static void expectInt(const std::string &name, long long actual, long long expected)
{
    if (actual == expected) gPassed++;
    else { gFailed++; std::cerr << "[FAIL] " << name << "：期望 " << expected << "，实际 " << actual << '\n'; }
}
static void expectBool(const std::string &name, bool actual, bool expected)
{
    expectInt(name, actual ? 1 : 0, expected ? 1 : 0);
}

static void testExtendedFeatures()
{
    // 基础 FIFO + size
    {
        dsaac::ExtendedQueue<int> q(10);
        q.push(1); q.push(2); q.push(3);
        expectInt("size==3", static_cast<long long>(q.size()), 3);
        expectInt("front==1 (FIFO)", q.front(), 1);
        expectInt("back==3", q.back(), 3);
        q.pop();
        expectInt("pop 后 front==2", q.front(), 2);
    }
    // clear
    {
        dsaac::ExtendedQueue<int> q(10);
        q.push(1); q.push(2);
        q.clear();
        expectBool("clear 后为空", q.empty(), true);
        expectInt("clear 后 size==0", static_cast<long long>(q.size()), 0);
    }
    // swap
    {
        dsaac::ExtendedQueue<int> a(10), b(10);
        a.push(1); a.push(2);
        b.push(9);
        a.swap(b);
        expectInt("swap 后 a.front==9", a.front(), 9);
        expectInt("swap 后 b.front==1", b.front(), 1);
    }
    // search
    {
        dsaac::ExtendedQueue<int> q(10);
        q.push(10); q.push(20); q.push(30);  // 队首 10
        expectInt("search(10)==1", q.search(10), 1);
        expectInt("search(30)==3", q.search(30), 3);
        expectInt("search(99)==-1", q.search(99), -1);
    }
    // 满队列上溢
    {
        dsaac::ExtendedQueue<int> q(2);
        q.push(1); q.push(2);
        bool threw = false;
        try { q.push(3); } catch (const std::overflow_error &) { threw = true; } catch (...) {}
        expectInt("满队列 push 抛 overflow_error", threw ? 1 : 0, 1);
    }
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
    testExtendedFeatures();
    std::cout << "# 扩展队列功能自测：通过 " << gPassed << " 项，失败 " << gFailed << " 项\n";

    const long long sizes[] = {100000, 1000000};
    const int REPEAT = 3;
    for (long long n : sizes) {
        double tExt = 0.0, tStd = 0.0;
        for (int r = 0; r < REPEAT; ++r) {
            tExt += measureTimeMs([&n]() {
                dsaac::ExtendedQueue<int> q(static_cast<std::size_t>(n));
                for (long long i = 0; i < n; ++i) q.push(static_cast<int>(i));
                long long s = 0;
                while (!q.empty()) { s += q.front(); q.pop(); }
                g_sink += s;
            });
            tStd += measureTimeMs([&n]() {
                std::queue<int> q;
                for (long long i = 0; i < n; ++i) q.push(static_cast<int>(i));
                long long s = 0;
                while (!q.empty()) { s += q.front(); q.pop(); }
                g_sink += s;
            });
        }
        tExt /= REPEAT; tStd /= REPEAT;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "PERF circular_queue_ext n=" << n << " ops=" << n * 2 << " time_ms=" << tExt << '\n';
        std::cout << "PERF std_queue n=" << n << " ops=" << n * 2 << " time_ms=" << tStd << '\n';
    }

    return gFailed > 0 ? 1 : 0;
}