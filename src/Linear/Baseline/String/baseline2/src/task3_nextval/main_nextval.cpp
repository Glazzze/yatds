/*
 * Baseline 2 Step 3：next / nextval 的差异与常数级优化。
 *
 * 1. 打印若干模式串的 next、NEXTVAL（课程口径）与扫描回退表；
 * 2. 构造“全 A 模式串 + 块状主串”：每次匹配到 m-1 个 A 后被 B 打断。
 *    原始 next 需要沿失败链连续回退 m-1 次，nextval 回退表可直接回到 0，
 *    用字符比较次数展示常数级改善。
 */
#include "../pattern_match/match.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

volatile long long gSink = 0;

std::string join(const std::vector<int> &values)
{
    std::string out;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0)
            out += ' ';
        out += std::to_string(values[i]);
    }
    return out;
}

double measureMs(const std::string &text, const std::string &pattern, bool useNextVal,
                 int reps)
{
    const auto start = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r) {
        gSink += useNextVal ? kmpNextValMatch(text, pattern) : kmpMatch(text, pattern);
    }
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count() / reps;
}

void dumpPattern(const std::string &pattern)
{
    std::cout << "PATTERN " << pattern << '\n';
    std::cout << "NEXT    " << join(getNext(pattern)) << '\n';
    std::cout << "NEXTVAL " << join(getNextVal(pattern)) << '\n';
    std::cout << "FALLBK  " << join(getNextValFallback(pattern)) << '\n';
}

} // namespace

int main()
{
    dumpPattern("AAAAB");
    dumpPattern("AAAAA");
    dumpPattern("ABAB");
    dumpPattern("AABAAAB");

    std::cout << "# 块状主串（每块 m-1 个 A 后接 B），模式串 A^m：next 需连续回退，"
                 "nextval 一步归零\n";

    const std::size_t m = 1000;
    std::string pattern(m, 'A');

    std::string text;
    while (text.size() + m < 2000000) {
        text.append(m - 1, 'A');
        text.push_back('B');
    }
    text.append(m - 1, 'A');

    long long kmpCmp = 0;
    long long nextCmp = 0;
    const int a = kmpMatch(text, pattern, &kmpCmp);
    const int b = kmpNextValMatch(text, pattern, &nextCmp);

    const double kmpMs = measureMs(text, pattern, false, 3);
    const double nextMs = measureMs(text, pattern, true, 3);

    std::cout << "PERF kmp n=" << text.size() << " m=" << m << " time_ms=" << std::fixed
              << std::setprecision(3) << kmpMs << " comparisons=" << kmpCmp
              << " match_pos=" << a << '\n';
    std::cout << "PERF kmp_nextval n=" << text.size() << " m=" << m
              << " time_ms=" << nextMs << " comparisons=" << nextCmp
              << " match_pos=" << b << '\n';
    if (kmpCmp > 0) {
        std::cout << "RATIO nextval/kmp comparisons = "
                  << static_cast<double>(nextCmp) / static_cast<double>(kmpCmp) << '\n';
    }
    return 0;
}