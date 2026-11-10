/*
 * Baseline 1 小任务 3：性能实验。
 *
 * 三个实验：
 *   1. 连续拼接 100 万次：MyString（2 倍扩容）/ MyString + reserve / std::string；
 *   2. 扩容策略对比：每次 +1、1.5 倍、2 倍，观察重分配次数与耗时随规模的变化；
 *   3. 大规模下 1.5 倍 / 2 倍 / reserve 的差异。
 *
 * 输出统一为 PERF 行，便于与实验报告的表格对应。
 * 计时使用 std::chrono::steady_clock，每个场景重复 kReps 次取算术平均。
 */
#include "../my_string/mystring.hpp"

#include <chrono>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

constexpr int kReps = 5;

/* 防止编译器把拼接循环优化掉：把最终长度累加到一个不可省略的观测值 */
volatile std::size_t gSink = 0;

double measureMs(const std::function<void()> &work)
{
    double total = 0.0;
    for (int r = 0; r < kReps; ++r) {
        const auto start = std::chrono::steady_clock::now();
        work();
        const auto end = std::chrono::steady_clock::now();
        total += std::chrono::duration<double, std::milli>(end - start).count();
    }
    return total / kReps;
}

void printPerf(const char *name, std::size_t n, std::size_t ops, double ms,
               long long reallocs = -1)
{
    std::cout << "PERF " << name << " n=" << n << " ops=" << ops << " time_ms=" << std::fixed
              << std::setprecision(3) << ms;
    if (reallocs >= 0)
        std::cout << " reallocs=" << reallocs;
    std::cout << '\n';
}

/* 连续追加 n 次单字符 */
void myStringAppend(std::size_t n, double growthFactor, bool reserveFirst)
{
    MyString s;
    s.setGrowthFactor(growthFactor);
    if (reserveFirst)
        s.reserve(n + 1);
    for (std::size_t i = 0; i < n; ++i)
        s.append("x");
    gSink += s.length();
}

void stdStringAppend(std::size_t n, bool reserveFirst)
{
    std::string s;
    if (reserveFirst)
        s.reserve(n + 1);
    for (std::size_t i = 0; i < n; ++i)
        s.append("x");
    gSink += s.length();
}

/* 返回 MyString 连续追加 n 次后的重分配次数（单独一次，不计时） */
long long countReallocs(std::size_t n, double growthFactor)
{
    MyString s;
    s.setGrowthFactor(growthFactor);
    for (std::size_t i = 0; i < n; ++i)
        s.append("x");
    gSink += s.length();
    return static_cast<long long>(s.reallocationCount());
}

} // namespace

int main()
{
    std::cout << "# 连续追加单字符，每次实验重复 " << kReps << " 次取平均\n";

    /* ---------- 实验 1：100 万次连续拼接 ---------- */
    const std::size_t n = 1000000;
    printPerf("my_string", n, n, measureMs([&]() { myStringAppend(n, 2.0, false); }));
    printPerf("std_string", n, n, measureMs([&]() { stdStringAppend(n, false); }));
    printPerf("my_string_reserve", n, n,
              measureMs([&]() { myStringAppend(n, 2.0, true); }));

    /* ---------- 实验 2：扩容策略随规模变化 ---------- */
    std::cout << "# 扩容策略对比：+1（exact）/1.5 倍/2 倍，规模逐级翻倍\n";
    const std::size_t sizes[] = {2000, 4000, 8000, 16000};
    for (std::size_t size : sizes) {
        const double tExact = measureMs([&]() { myStringAppend(size, 0.0, false); });
        const long long rExact = countReallocs(size, 0.0);
        printPerf("growth_exact", size, size, tExact, rExact);

        const double t15 = measureMs([&]() { myStringAppend(size, 1.5, false); });
        printPerf("growth_1_5x", size, size, t15, countReallocs(size, 1.5));

        const double t2 = measureMs([&]() { myStringAppend(size, 2.0, false); });
        printPerf("growth_2x", size, size, t2, countReallocs(size, 2.0));
    }

    /* ---------- 实验 3：百万规模下的等比扩容与预留 ---------- */
    std::cout << "# 百万规模：等比扩容与 reserve 对比\n";
    printPerf("million_1_5x", n, n, measureMs([&]() { myStringAppend(n, 1.5, false); }));
    printPerf("million_2x", n, n, measureMs([&]() { myStringAppend(n, 2.0, false); }));
    printPerf("million_reserve", n, n,
              measureMs([&]() { myStringAppend(n, 2.0, true); }));
    printPerf("million_std", n, n, measureMs([&]() { stdStringAppend(n, false); }));

    return 0;
}