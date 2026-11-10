/*
 * Baseline 3（命令式测试程序，提交名 test）。
 *
 * 输入：第 1 行 DNA 序列 S，第 2 行目标蛋白 P，第 3 行查询次数 q，
 *       随后 q 行 l r。
 * 输出：第 1 行 P 的前缀函数 pi；第 2 行匹配数 K；随后 K 行
 *       "L R strand frame"（1-based 闭区间）；最后 q 行查询答案。
 */
#include "../dna_search/dna_search.hpp"

#include <dirent.h>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string joinArray(const std::vector<int> &values)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0)
            oss << ' ';
        oss << values[i];
    }
    return oss.str();
}

std::string runCase(std::istream &fin)
{
    std::ostringstream out;

    std::string dna;
    std::string protein;
    if (!std::getline(fin, dna))
        return out.str();
    if (!std::getline(fin, protein))
        return out.str();
    if (!dna.empty() && dna.back() == '\r')
        dna.pop_back();
    if (!protein.empty() && protein.back() == '\r')
        protein.pop_back();

    int q = 0;
    if (!(fin >> q))
        return out.str();

    std::vector<std::pair<int, int>> queries;
    queries.reserve(q);
    for (int i = 0; i < q; ++i) {
        int l = 0;
        int r = 0;
        if (!(fin >> l >> r))
            break;
        queries.emplace_back(l, r);
    }

    const std::vector<ProteinMatch> matches = findProteinMatches(dna, protein);
    const std::vector<long long> answers =
        answerQueries(matches, static_cast<int>(dna.size()),
                      static_cast<int>(protein.size()), queries);

    out << joinArray(prefixFunction(protein)) << '\n';
    out << matches.size() << '\n';
    for (const ProteinMatch &m : matches)
        out << m.l << ' ' << m.r << ' ' << m.strand << ' ' << m.frame << '\n';
    for (long long answer : answers)
        out << answer << '\n';

    return out.str();
}

std::string readFile(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return std::string();
    std::ostringstream oss;
    oss << f.rdbuf();
    return oss.str();
}

std::vector<std::string> listCaseFiles(const std::string &dir)
{
    std::vector<std::string> files;
    DIR *dp = opendir(dir.c_str());
    if (dp == nullptr)
        return files;
    struct dirent *entry;
    while ((entry = readdir(dp)) != nullptr) {
        std::string name = entry->d_name;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt")
            files.push_back(name);
    }
    closedir(dp);
    std::sort(files.begin(), files.end());
    return files;
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "用法:\n"
                  << "  ./test                    自动遍历 inputs/ 与 outputs/，逐用例比对\n"
                  << "  ./test inputs/case01.txt  单用例模式：打印该样例的程序输出\n";
        return 0;
    }

    if (argc > 1) {
        std::ifstream fin(argv[1]);
        if (!fin.is_open()) {
            std::cerr << "无法打开输入文件: " << argv[1] << '\n';
            return 1;
        }
        std::cout << runCase(fin);
        return 0;
    }

    const std::string inputDir = "inputs";
    const std::string outputDir = "outputs";
    const std::vector<std::string> cases = listCaseFiles(inputDir);
    if (cases.empty()) {
        std::cerr << "inputs/ 目录中没有找到 .txt 样例\n";
        return 1;
    }

    int passed = 0;
    for (const std::string &name : cases) {
        std::ifstream fin(inputDir + "/" + name);
        if (!fin.is_open()) {
            std::cout << name << ": FAIL (无法打开输入文件)\n";
            continue;
        }
        const std::string expected = readFile(outputDir + "/" + name);
        const std::string actual = runCase(fin);
        if (actual == expected) {
            std::cout << name << ": PASS\n";
            ++passed;
        } else {
            std::cout << name << ": FAIL\n";
            std::cout << "--- 期望 ---\n" << expected;
            std::cout << "--- 实际 ---\n" << actual;
        }
    }

    std::cout << "共 " << cases.size() << " 个用例，通过 " << passed << " 个\n";
    return passed == static_cast<int>(cases.size()) ? 0 : 1;
}