/*
 * Baseline 2 小任务 2/3：KMP 与 nextval 的单元验证。
 *
 * 断言内容：
 *   - next 数组与课程样例一致（AAAAB -> 0 1 2 3 0）；
 *   - NEXTVAL 与课程样例一致（AAAAB -> 0 0 0 0 0）；
 *   - 扫描回退表在极端重复串上能正确工作；
 *   - 朴素 / KMP / KMP+nextval 三种算法在大量用例上结果一致；
 *   - 最坏情况下 KMP 的字符比较次数显著少于朴素算法。
 */
#include "../pattern_match/match.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int gPassed = 0;
int gFailed = 0;

void check(bool cond, const std::string &what)
{
    if (cond) {
        ++gPassed;
    } else {
        ++gFailed;
        std::cout << "[FAIL] " << what << '\n';
    }
}

std::string join(const std::vector<int> &values)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0)
            oss << ' ';
        oss << values[i];
    }
    return oss.str();
}

void checkArray(const std::vector<int> &actual, const std::string &expected,
                const std::string &what)
{
    if (join(actual) == expected) {
        ++gPassed;
    } else {
        ++gFailed;
        std::cout << "[FAIL] " << what << "：期望 [" << expected << "]，实际 ["
                  << join(actual) << "]\n";
    }
}

/* 独立于实现的三方对照：逐位置暴力比较，作为正确性基准 */
int referenceMatch(const std::string &text, const std::string &pattern)
{
    if (pattern.empty())
        return 0;
    if (pattern.size() > text.size())
        return -1;
    for (std::size_t i = 0; i + pattern.size() <= text.size(); ++i) {
        std::size_t j = 0;
        while (j < pattern.size() && text[i + j] == pattern[j])
            ++j;
        if (j == pattern.size())
            return static_cast<int>(i);
    }
    return -1;
}

void checkAllSame(const std::string &text, const std::string &pattern)
{
    const int expected = referenceMatch(text, pattern);
    const int a = naiveMatch(text, pattern);
    const int b = kmpMatch(text, pattern);
    const int c = kmpNextValMatch(text, pattern);

    const std::string what = "三种算法与基准一致: text=\"" + text + "\", pattern=\"" +
                             pattern + "\"";
    const bool ok = (a == expected) && (b == expected) && (c == expected);
    if (ok) {
        ++gPassed;
    } else {
        ++gFailed;
        std::cout << "[FAIL] " << what << "：基准 " << expected << "，naive " << a
                  << "，kmp " << b << "，nextval " << c << '\n';
    }
}

} // namespace

int main()
{
    /* ---------- 1. next 数组 ---------- */
    checkArray(getNext("AAAAB"), "0 1 2 3 0", "getNext(AAAAB)");
    checkArray(getNext("ABAB"), "0 0 1 2", "getNext(ABAB)");
    checkArray(getNext("AAAAA"), "0 1 2 3 4", "getNext(AAAAA)");
    checkArray(getNext("ABCDE"), "0 0 0 0 0", "getNext(无重复)");
    checkArray(getNext(""), "", "getNext(空模式)");

    /* ---------- 2. NEXTVAL（课程样例口径） ---------- */
    checkArray(getNextVal("AAAAB"), "0 0 0 0 0", "getNextVal(AAAAB) 与课程样例一致");
    checkArray(getNextVal("ABAB"), "0 0 1 2", "getNextVal(ABAB)");
    checkArray(getNextVal("AAAAA"), "0 0 0 0 0", "getNextVal(AAAAA) 连续相同字符全部跳过");
    checkArray(getNextVal("AABAAAB"), "0 0 0 0 2 2 3", "getNextVal(AABAAAB)");

    /* ---------- 3. 扫描安全回退表 ---------- */
    checkArray(getNextValFallback("AAAAB"), "0 0 0 0 3", "fallback(AAAAB) 末位保留 3");
    checkArray(getNextValFallback("ABAB"), "0 0 0 0", "fallback(ABAB)");
    checkArray(getNextValFallback("AAAAA"), "0 0 0 0 0", "fallback(AAAAA)");

    /* ---------- 4. 三种算法结果一致 ---------- */
    const std::vector<std::pair<std::string, std::string>> cases = {
        {"AAAAAAAAAAB", "AAAAB"},   // 官方样例
        {"AAAAAAAAAAB", "AAAAA"},
        {"ABABABAB", "ABAB"},
        {"ABCABCABC", "ABCD"},
        {"ABC", "ABCDEFG"},         // 模式长于主串
        {"ABC", "ABC"},
        {"ABC", "C"},
        {"ABC", "BC"},
        {"", "A"},                  // 空主串
        {"", ""},                   // 双空
        {"ABC", ""},                // 空模式
        {"AAAAA", "AA"},
        {"mississippi", "issi"},
        {"mississippi", "issip"},
        {"abcbcabcabcd", "abcd"},
        {"aabaabaaa", "aabaaa"},
        {"ABABDABACDABABCABAB", "ABABCABAB"},
    };
    for (const auto &c : cases)
        checkAllSame(c.first, c.second);

    /* ---------- 5. 最坏情况下 KMP 的比较次数优势 ---------- */
    {
        const int n = 2000;
        const int m = 200;
        std::string text(n, 'A');
        text.back() = 'B';
        std::string pattern(m, 'A');
        pattern.back() = 'B';

        long long naiveCmp = 0;
        long long kmpCmp = 0;
        long long nextCmp = 0;
        const int naivePos = naiveMatch(text, pattern, &naiveCmp);
        const int kmpPos = kmpMatch(text, pattern, &kmpCmp);
        const int nextPos = kmpNextValMatch(text, pattern, &nextCmp);

        check(naivePos == kmpPos && kmpPos == nextPos, "极端用例三种算法结果一致");
        check(naivePos == n - m, "极端用例匹配位置正确");
        check(kmpCmp < naiveCmp / 10, "KMP 比较次数远少于朴素算法");
        check(nextCmp <= kmpCmp, "nextval 比较次数不多于 KMP");
    }

    /* ---------- 6. 连续相同字符的 nextval 压缩效果 ---------- */
    {
        const std::string pattern(100, 'A');
        const std::vector<int> nv = getNextVal(pattern);
        bool allZero = true;
        for (int v : nv)
            allZero = allZero && (v == 0);
        check(allZero, "全相同字符模式串的 nextval 全为 0");

        const std::vector<int> fb = getNextValFallback(pattern);
        bool fallbackOk = true;
        for (std::size_t i = 1; i < fb.size(); ++i)
            fallbackOk = fallbackOk && (fb[i] < static_cast<int>(i));
        check(fallbackOk, "扫描回退表每项严格小于当前长度");
    }

    std::cout << "模式匹配单元验证：通过 " << gPassed << " 项，失败 " << gFailed << " 项\n";
    return gFailed == 0 ? 0 : 1;
}