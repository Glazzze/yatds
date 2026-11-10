#ifndef DSAAC_PATTERN_MATCH_HPP
#define DSAAC_PATTERN_MATCH_HPP

#include <string>
#include <vector>

namespace dsaac {

/*
 * 模式匹配（Baseline 2）。
 *
 * 下标约定：匹配成功返回主串中首次匹配的起始下标（0 开始），失败返回 -1；
 * 空模式串视为在位置 0 匹配成功（与 std::string::find 一致）。
 *
 * next 数组采用课程骨架的“前缀函数”约定：
 *   next[j] = 模式串 P[0..j] 的最长真前缀同时也是后缀的长度（next[0] = 0）。
 *
 * 本文件提供两个“优化表”：
 *   - getNextVal：按课程官方样例的口径输出 NEXTVAL（如 AAAAB -> 0 0 0 0 0），
 *     用于命令式测试的 NEXTVAL 操作；
 *   - getNextValFallback：可直接用于扫描的真·失败回退表。二者在部分模式上
 *     不同，原因见 baseline2/README.md 的“两套 nextval”说明。
 */

int naiveMatch(const std::string &text, const std::string &pattern,
               long long *comparisons = nullptr);
std::vector<int> getNext(const std::string &pattern);
int kmpMatch(const std::string &text, const std::string &pattern,
             long long *comparisons = nullptr);

/* Step 3 进阶优化要求：nextval 数组（课程样例口径） */
std::vector<int> getNextVal(const std::string &pattern);

/* 可直接用于 KMP 扫描的失败回退表（nextval 的扫描安全形式） */
std::vector<int> getNextValFallback(const std::string &pattern);

/* 使用 nextval 回退表进行匹配 */
int kmpNextValMatch(const std::string &text, const std::string &pattern,
                    long long *comparisons = nullptr);

} // namespace dsaac

// 为了方便使用，将方法暴露在全局（也可按需不暴露）
using dsaac::naiveMatch;
using dsaac::getNext;
using dsaac::kmpMatch;
using dsaac::getNextVal;
using dsaac::getNextValFallback;
using dsaac::kmpNextValMatch;

#endif // DSAAC_PATTERN_MATCH_HPP