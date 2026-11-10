/*
 * Baseline 2 Step 1：朴素算法在最坏情况下的性能。
 *
 * 构造最坏用例：text = 'A'^(n-1) + 'B'，pattern = 'A'^(m-1) + 'B'。
 * 每次尝试都要比对 m 个字符、直到最后一个字符才失配，朴素算法退化为 O(nm)。
 * 输出随规模翻倍的耗时与比较次数，用于验证二次增长趋势。
 */
#include "../pattern_match/match.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

volatile long long gSink = 0;

double measureMs(const std::string &text, const std::string &pattern, int reps)
{
    const auto start = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r)
        gSink += naiveMatch(text, pattern);
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count() / reps;
}

double measureKmpMs(const std::string &text, const std::string &pattern, int reps)
{
    const auto start = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r)
        gSink += kmpMatch(text, pattern);
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count() / reps;
}

} // namespace

int main()
{
    std::cout << "# 最坏用例：'A'^(n-1)+'B' 对 'A'^(m-1)+'B'，每次尝试到末尾才失配\n";
    const std::size_t ns[] = {10000, 20000, 40000};
    const std::size_t ms[] = {100, 200, 400};

    for (int k = 0; k < 3; ++k) {
        std::string text(ns[k], 'A');
        text.back() = 'B';
        std::string pattern(ms[k], 'A');
        pattern.back() = 'B';

        long long comparisons = 0;
        const int pos = naiveMatch(text, pattern, &comparisons);
        const double msTime = measureMs(text, pattern, 3);

        std::cout << "PERF naive_growth n=" << ns[k] << " m=" << ms[k]
                  << " time_ms=" << std::fixed << std::setprecision(3) << msTime
                  << " comparisons=" << comparisons << " match_pos=" << pos << '\n';

        long long kmpComparisons = 0;
        const int kmpPos = kmpMatch(text, pattern, &kmpComparisons);
        const double kmpTime = measureKmpMs(text, pattern, 3);
        std::cout << "PERF kmp_growth n=" << ns[k] << " m=" << ms[k]
                  << " time_ms=" << kmpTime
                  << " comparisons=" << kmpComparisons << " match_pos=" << kmpPos << '\n';
    }
    return 0;
}