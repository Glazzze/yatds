/*
 * Baseline 2 小任务 4（命令式测试程序，提交名 test）。
 *
 * 输入格式（课程 README）：
 *   第 1 行：text（主串）
 *   第 2 行：pattern（模式串）
 *   第 3 行：m（操作次数）
 *   接下来 m 行：NEXT / NEXTVAL / NAIVE / KMP / KMP_NEXTVAL / ALL
 *
 * 输出：数组用单个空格分隔；匹配下标从 0 开始，不存在输出 -1；
 *       功能输出后追加 PERF naive / kmp / kmp_nextval 三行。
 *
 * 比对约定：功能行逐字符精确比对；PERF 行只校验名称与格式。
 */
#include "../pattern_match/match.hpp"

#include <dirent.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

volatile long long gSink = 0;

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

template <typename F>
double timePerRunMs(F work, int reps)
{
    const auto start = std::chrono::steady_clock::now();
    for (int r = 0; r < reps; ++r)
        gSink += work();
    const auto end = std::chrono::steady_clock::now();
    const double total = std::chrono::duration<double, std::milli>(end - start).count();
    return total / reps;
}

/* 运行单个用例，返回完整输出（功能行 + PERF 行） */
std::string runCase(std::istream &fin)
{
    std::ostringstream out;

    std::string text;
    std::string pattern;
    if (!std::getline(fin, text))
        return out.str();
    if (!std::getline(fin, pattern))
        return out.str();
    if (!text.empty() && text.back() == '\r')
        text.pop_back();
    if (!pattern.empty() && pattern.back() == '\r')
        pattern.pop_back();

    int m = 0;
    if (!(fin >> m))
        return out.str();
    fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    const std::vector<int> next = getNext(pattern);
    const std::vector<int> nextval = getNextVal(pattern);

    for (int i = 0; i < m; ++i) {
        std::string op;
        if (!(fin >> op))
            break;
        if (op == "NEXT") {
            out << joinArray(next) << '\n';
        } else if (op == "NEXTVAL") {
            out << joinArray(nextval) << '\n';
        } else if (op == "NAIVE") {
            out << naiveMatch(text, pattern) << '\n';
        } else if (op == "KMP") {
            out << kmpMatch(text, pattern) << '\n';
        } else if (op == "KMP_NEXTVAL") {
            out << kmpNextValMatch(text, pattern) << '\n';
        } else if (op == "ALL") {
            out << naiveMatch(text, pattern) << '\n';
            out << kmpMatch(text, pattern) << '\n';
            out << kmpNextValMatch(text, pattern) << '\n';
        }
    }

    /* 性能统计：按输入规模自适应重复次数，降低计时噪声 */
    const std::size_t n = text.size();
    const std::size_t pm = pattern.size();
    int reps = 1;
    const std::size_t scale = n + pm + 1;
    if (scale < 10000)
        reps = static_cast<int>(50000 / scale);
    if (reps < 1)
        reps = 1;

    const double naiveMs =
        timePerRunMs([&]() { return naiveMatch(text, pattern); }, reps);
    const double kmpMs = timePerRunMs([&]() { return kmpMatch(text, pattern); }, reps);
    const double nextMs =
        timePerRunMs([&]() { return kmpNextValMatch(text, pattern); }, reps);

    out << std::fixed << std::setprecision(6);
    out << "PERF naive n=" << n << " m=" << pm << " time_ms=" << naiveMs << '\n';
    out << "PERF kmp n=" << n << " m=" << pm << " time_ms=" << kmpMs << '\n';
    out << "PERF kmp_nextval n=" << n << " m=" << pm << " time_ms=" << nextMs << '\n';

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

bool isPerfLine(const std::string &line)
{
    return line.rfind("PERF ", 0) == 0;
}

void splitOutput(const std::string &text, std::vector<std::string> &functional,
                 std::vector<std::string> &perf)
{
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (isPerfLine(line))
            perf.push_back(line);
        else
            functional.push_back(line);
    }
}

bool perfMatches(const std::string &actual, const std::string &expected)
{
    std::istringstream ae(actual);
    std::istringstream ee(expected);
    std::string nameA;
    std::string nameE;
    std::string ignore;
    if (!(ae >> ignore >> nameA) || !(ee >> ignore >> nameE))
        return false;
    if (nameA != nameE)
        return false;
    return actual.find(" n=") != std::string::npos &&
           actual.find(" m=") != std::string::npos &&
           actual.find(" time_ms=") != std::string::npos;
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "用法:\n"
                  << "  ./test                    自动遍历 inputs/ 与 outputs/，逐用例比对\n"
                  << "  ./test inputs/case01.txt  单用例模式：打印完整输出（含 PERF 行）\n";
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

        std::vector<std::string> fActual;
        std::vector<std::string> pActual;
        std::vector<std::string> fExpected;
        std::vector<std::string> pExpected;
        splitOutput(actual, fActual, pActual);
        splitOutput(expected, fExpected, pExpected);

        bool ok = (fActual == fExpected) && (pActual.size() == pExpected.size());
        if (ok) {
            for (std::size_t i = 0; i < pActual.size(); ++i) {
                if (!perfMatches(pActual[i], pExpected[i])) {
                    ok = false;
                    break;
                }
            }
        }

        if (ok) {
            std::cout << name << ": PASS\n";
            ++passed;
        } else {
            std::cout << name << ": FAIL\n";
            std::cout << "--- 期望（功能行）---\n";
            for (const std::string &line : fExpected)
                std::cout << line << '\n';
            std::cout << "--- 实际（功能行）---\n";
            for (const std::string &line : fActual)
                std::cout << line << '\n';
        }
    }

    std::cout << "共 " << cases.size() << " 个用例，通过 " << passed << " 个\n";
    return passed == static_cast<int>(cases.size()) ? 0 : 1;
}