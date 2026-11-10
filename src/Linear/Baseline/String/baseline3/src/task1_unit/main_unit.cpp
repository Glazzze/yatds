/*
 * Baseline 3 单元验证：遗传密码表、反向互补、翻译、KMP 匹配、
 * 6 个阅读框的坐标映射与区间查询前缀和。
 */
#include "../dna_search/dna_search.hpp"

#include <iostream>
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

void checkEq(const std::string &actual, const std::string &expected, const std::string &what)
{
    if (actual == expected) {
        ++gPassed;
    } else {
        ++gFailed;
        std::cout << "[FAIL] " << what << "：期望 \"" << expected << "\"，实际 \"" << actual
                  << "\"\n";
    }
}


std::string matchToString(const ProteinMatch &m)
{
    std::string s = std::to_string(m.l) + " " + std::to_string(m.r) + " " + m.strand + " " +
                    std::to_string(m.frame);
    return s;
}

bool equals(const std::vector<ProteinMatch> &actual, const std::vector<std::string> &expected)
{
    if (actual.size() != expected.size())
        return false;
    for (std::size_t i = 0; i < actual.size(); ++i) {
        if (matchToString(actual[i]) != expected[i])
            return false;
    }
    return true;
}

} // namespace

int main()
{
    /* ---------- 1. 遗传密码表 ---------- */
    checkEq(std::string(1, translateCodon("ATG")), "M", "ATG -> M");
    checkEq(std::string(1, translateCodon("GCT")), "A", "GCT -> A");
    checkEq(std::string(1, translateCodon("GCC")), "A", "GCC -> A");
    checkEq(std::string(1, translateCodon("GCA")), "A", "GCA -> A");
    checkEq(std::string(1, translateCodon("GCG")), "A", "GCG -> A");
    checkEq(std::string(1, translateCodon("TAA")), "*", "TAA -> *");
    checkEq(std::string(1, translateCodon("TAG")), "*", "TAG -> *");
    checkEq(std::string(1, translateCodon("TGA")), "*", "TGA -> *");
    checkEq(std::string(1, translateCodon("TGG")), "W", "TGG -> W");
    checkEq(std::string(1, translateCodon("NNN")), "X", "非法密码子 -> X");

    /* ---------- 2. 反向互补 ---------- */
    checkEq(reverseComplement("AATGGCC"), "GGCCATT", "反向互补 AATGGCC");
    checkEq(reverseComplement("ATGC"), "GCAT", "反向互补 ATGC");
    checkEq(reverseComplement(""), "", "反向互补空串");
    {
        const std::string dna = "AATGGCTTTAAGCCATCATGGCC";
        checkEq(reverseComplement(reverseComplement(dna)), dna, "反向互补两次还原");
    }

    /* ---------- 3. 翻译阅读框 ---------- */
    checkEq(translateFrame("ATGATGATG", 0), "MMM", "frame 0 翻译");
    checkEq(translateFrame("ATGATGATG", 1), "**", "frame 1 翻译并丢弃不足 3 碱基的尾部");
    checkEq(translateFrame("ATGATG", 2), "D", "frame 2 翻译");
    checkEq(translateFrame("AT", 0), "", "不足一个密码子返回空串");

    /* ---------- 4. 前缀函数 / 全匹配 ---------- */
    {
        const std::vector<int> pi = prefixFunction("AAAAB");
        check(pi == std::vector<int>({0, 1, 2, 3, 0}), "prefixFunction(AAAAB)");
    }
    {
        const std::vector<std::size_t> pos = kmpFindAll("AAAAA", "AA", prefixFunction("AA"));
        check(pos == std::vector<std::size_t>({0, 1, 2, 3}), "KMP 全匹配（允许重叠）");
    }
    {
        const std::vector<std::size_t> pos =
            kmpFindAll("ABABAB", "ABAB", prefixFunction("ABAB"));
        check(pos == std::vector<std::size_t>({0, 2}), "KMP 重叠匹配 ABAB");
    }

    /* ---------- 5. 官方样例端到端 ---------- */
    {
        const std::string dna = "AATGGCTTTAAGCCATCATGGCC";
        const std::string protein = "MA";
        const std::vector<ProteinMatch> matches = findProteinMatches(dna, protein);
        check(equals(matches, {"2 7 + 2", "11 16 - 2", "18 23 + 3"}),
              "官方样例三个匹配片段与坐标");

        const std::vector<std::pair<int, int>> queries = {
            {1, 7}, {1, 16}, {10, 16}, {12, 23}, {1, 23}, {3, 17}};
        const std::vector<long long> answers =
            answerQueries(matches, static_cast<int>(dna.size()), static_cast<int>(protein.size()),
                          queries);
        check(answers == std::vector<long long>({1, 2, 1, 1, 3, 1}), "官方样例六个查询答案");
    }

    /* ---------- 6. 重叠匹配 ---------- */
    {
        const std::vector<ProteinMatch> matches = findProteinMatches("ATGATGATG", "M");
        check(equals(matches, {"1 3 + 1", "4 6 + 1", "7 9 + 1"}), "正向重叠匹配");

        const std::vector<std::pair<int, int>> queries = {{1, 3}, {2, 3}, {1, 9}, {2, 9}, {4, 9}};
        const std::vector<long long> answers = answerQueries(matches, 9, 1, queries);
        check(answers == std::vector<long long>({1, 0, 3, 2, 2}), "重叠匹配的区间查询");
    }

    /* ---------- 7. 仅反向链命中 ---------- */
    {
        const std::vector<ProteinMatch> matches = findProteinMatches("CAT", "M");
        check(equals(matches, {"1 3 - 1"}), "只通过反向互补链命中");
    }

    /* ---------- 8. 边界 ---------- */
    {
        check(findProteinMatches("AAA", "W").empty(), "无匹配返回空");
        check(findProteinMatches("ATGATG", "WWWW").empty(), "目标蛋白长于阅读框返回空");
        check(findProteinMatches("ATG", "").empty(), "空目标蛋白不产生匹配");

        const std::vector<ProteinMatch> none;
        const std::vector<std::pair<int, int>> queries = {{1, 3}, {0, 0}, {4, 2}, {-5, 2}};
        const std::vector<long long> answers = answerQueries(none, 3, 1, queries);
        check(answers == std::vector<long long>({0, 0, 0, 0}), "无匹配 / 非法查询均返回 0");
    }

    std::cout << "DNA 检索单元验证：通过 " << gPassed << " 项，失败 " << gFailed << " 项\n";
    return gFailed == 0 ? 0 : 1;
}