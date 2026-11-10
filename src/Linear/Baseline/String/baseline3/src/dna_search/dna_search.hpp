#ifndef DSAAC_DNA_SEARCH_HPP
#define DSAAC_DNA_SEARCH_HPP

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace dsaac {

/* 一个蛋白质匹配片段：原 DNA 序列上的 1-based 闭区间 [l, r] */
struct ProteinMatch {
    int l;        // 左端点（1-based）
    int r;        // 右端点（1-based，含）
    char strand;  // '+' 正向链，'-' 反向互补链
    int frame;    // 阅读框编号 1 / 2 / 3
};

/* KMP 前缀函数：pi[j] = P[0..j] 的最长真前缀同时也是后缀的长度 */
std::vector<int> prefixFunction(const std::string &pattern);

/* 在主串中查找 pattern 的全部出现位置（允许重叠），返回 0-based 起始下标 */
std::vector<std::size_t> kmpFindAll(const std::string &text, const std::string &pattern,
                                    const std::vector<int> &pi);

/* 反向互补：先反转，再 A<->T、C<->G */
std::string reverseComplement(const std::string &dna);

/* 将一个密码子（3 个碱基）翻译为氨基酸；终止密码子返回 '*' */
char translateCodon(const std::string &codon);

/* 按阅读框（0/1/2）把 DNA 翻译为蛋白质序列；不足 3 个碱基的尾部丢弃 */
std::string translateFrame(const std::string &dna, int frame);

/*
 * 在正/反两条链、各 3 个阅读框中查找目标蛋白质的全部匹配，
 * 并把坐标映射回原 DNA 序列（1-based 闭区间）。
 */
std::vector<ProteinMatch> findProteinMatches(const std::string &dna,
                                             const std::string &protein);

/*
 * 对每个查询 [l, r] 回答“完整包含的匹配片段个数”。
 * 使用前缀和预处理，单次查询 O(1)。
 */
std::vector<long long> answerQueries(const std::vector<ProteinMatch> &matches,
                                     int dnaLength, int proteinLength,
                                     const std::vector<std::pair<int, int>> &queries);

} // namespace dsaac

using dsaac::ProteinMatch;
using dsaac::prefixFunction;
using dsaac::kmpFindAll;
using dsaac::reverseComplement;
using dsaac::translateCodon;
using dsaac::translateFrame;
using dsaac::findProteinMatches;
using dsaac::answerQueries;

#endif // DSAAC_DNA_SEARCH_HPP