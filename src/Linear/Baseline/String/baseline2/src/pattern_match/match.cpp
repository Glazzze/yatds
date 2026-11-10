#include "match.hpp"

/* START: pattern_match implementation */

namespace {

/* 统计一次字符比较：cmp 为空时不计数 */
inline void countCompare(long long *cmp)
{
    if (cmp != nullptr)
        ++*cmp;
}

} // namespace

int dsaac::naiveMatch(const std::string &text, const std::string &pattern,
                      long long *comparisons)
{
    const int n = static_cast<int>(text.length());
    const int m = static_cast<int>(pattern.length());

    if (m == 0)
        return 0;

    for (int i = 0; i <= n - m; i++) {
        int j = 0;
        while (j < m) {
            countCompare(comparisons);
            if (text[i + j] != pattern[j])
                break;
            j++;
        }
        if (j == m)
            return i; // 匹配成功，返回起点
    }
    return -1; // 匹配失败
}

std::vector<int> dsaac::getNext(const std::string &pattern)
{
    const int m = static_cast<int>(pattern.length());
    std::vector<int> next(m, 0);

    if (m == 0)
        return next;

    int head = 0;
    for (int tail = 1; tail < m; tail++) {
        while (head > 0 && pattern[head] != pattern[tail]) {
            head = next[head - 1];
        }
        if (pattern[head] == pattern[tail]) {
            head++;
        }
        next[tail] = head;
    }
    return next;
}

int dsaac::kmpMatch(const std::string &text, const std::string &pattern,
                    long long *comparisons)
{
    const int n = static_cast<int>(text.length());
    const int m = static_cast<int>(pattern.length());
    if (m == 0)
        return 0;
    if (n < m)
        return -1;

    const std::vector<int> next = getNext(pattern);
    int matched = 0; // 当前已匹配的模式串长度，也即下一次要比较 pattern[matched]

    for (int i = 0; i < n; ++i) {
        /* 失配时沿 next 回退；主串指针 i 不回退，这是 KMP 线性时间的来源 */
        while (matched > 0) {
            countCompare(comparisons);
            if (text[i] == pattern[matched])
                break;
            matched = next[matched - 1];
        }
        if (matched == 0) {
            countCompare(comparisons);
            if (text[i] == pattern[0])
                ++matched;
        } else {
            ++matched; // 上面的循环已经确认 text[i] == pattern[matched]
        }
        if (matched == m)
            return i - m + 1;
    }
    return -1;
}

std::vector<int> dsaac::getNextVal(const std::string &pattern)
{
    /*
     * 课程官方样例口径（AAAAB -> 0 0 0 0 0）：
     *   nextval[0] = 0；
     *   k = next[j]；
     *   若 k > 0 且 pattern[j] == pattern[k]，说明失配后跳到 k 还会因同一字符再次失配，
     *   于是继承 nextval[k-1]；否则 nextval[j] = k。
     *
     * 注意：该数组用于“展示 nextval 的跳过思想”，并不能直接作为前缀函数式
     * KMP 的失败指针（AAAAB 的末位 0 会让扫描漏掉正确匹配）。扫描使用
     * getNextValFallback 得到的回退表，详见 README。
     */
    const int m = static_cast<int>(pattern.length());
    std::vector<int> nextval(m, 0);
    if (m == 0)
        return nextval;

    const std::vector<int> next = getNext(pattern);
    for (int j = 1; j < m; ++j) {
        const int k = next[j];
        if (k > 0 && pattern[j] == pattern[k])
            nextval[j] = nextval[k - 1];
        else
            nextval[j] = k;
    }
    return nextval;
}

std::vector<int> dsaac::getNextValFallback(const std::string &pattern)
{
    /*
     * 扫描安全的失败回退表：
     *   回退[j] 表示“已匹配 j 个字符、比较 pattern[j] 失配”时应回退到的已匹配长度。
     *   从 k = next[j-1]（P[0..j-1] 的最长真前后缀）出发，若 pattern[j] == pattern[k]，
     *   那么回退到 k 后必然再次失配（因为 text 字符 != pattern[j]），继续沿 next 回退，
     *   直到 k == 0 或 pattern[j] != pattern[k]。
     */
    const int m = static_cast<int>(pattern.length());
    std::vector<int> fallback(m, 0);
    if (m == 0)
        return fallback;

    const std::vector<int> next = getNext(pattern);
    for (int j = 1; j < m; ++j) {
        int k = next[j - 1];
        while (k > 0 && pattern[j] == pattern[k])
            k = next[k - 1];
        fallback[j] = k;
    }
    return fallback;
}

int dsaac::kmpNextValMatch(const std::string &text, const std::string &pattern,
                           long long *comparisons)
{
    const int n = static_cast<int>(text.length());
    const int m = static_cast<int>(pattern.length());
    if (m == 0)
        return 0;
    if (n < m)
        return -1;

    const std::vector<int> fallback = getNextValFallback(pattern);
    int matched = 0;

    for (int i = 0; i < n; ++i) {
        while (matched > 0) {
            countCompare(comparisons);
            if (text[i] == pattern[matched])
                break;
            matched = fallback[matched];
        }
        if (matched == 0) {
            countCompare(comparisons);
            if (text[i] == pattern[0])
                ++matched;
        } else {
            ++matched;
        }
        if (matched == m)
            return i - m + 1;
    }
    return -1;
}

/* END */