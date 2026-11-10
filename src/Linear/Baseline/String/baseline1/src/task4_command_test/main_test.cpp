/*
 * Baseline 1 小任务 4（命令式测试程序，提交名 test）。
 *
 * 严格按课程 README 规定的输入输出格式读取 inputs/ 中的样例：
 *   第 1 行：初始字符串 s（空字符串写 EMPTY）
 *   第 2 行：操作次数 m
 *   接下来 m 行：op args...
 *
 * 操作：PRINT / LENGTH / APPEND t / SUBSTR pos len / FIND t /
 *       REPLACE old new / COMPARE t / CLEAR
 *
 * 输出约定：
 *   - PRINT 空串输出 EMPTY；
 *   - SUBSTR 越界输出 ERROR（非空串 pos >= length，或 pos/len 为负）；
 *   - FIND 不存在输出 -1；COMPARE 输出 0 / -1 / 1；
 *   - REPLACE 只替换首次出现；CLEAR 无输出。
 *   - 功能输出之后追加 PERF 三行（README 要求）。
 *
 * 比对约定：功能行逐字符精确比对；PERF 行只校验名称与格式，
 * 不比较随时间变化的 time_ms 数值。
 */
#include "../my_string/mystring.hpp"

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

/* 单次性能统计（同一进程内只测一次，供所有样例复用） */
struct PerfSample {
    double myMs;
    double stdMs;
    double reserveMs;
};

double appendBenchMy(std::size_t n, bool reserveFirst)
{
    MyString s;
    if (reserveFirst)
        s.reserve(n + 1);
    for (std::size_t i = 0; i < n; ++i)
        s.append("x");
    return static_cast<double>(s.length());
}

double appendBenchStd(std::size_t n)
{
    std::string s;
    for (std::size_t i = 0; i < n; ++i)
        s.append("x");
    return static_cast<double>(s.length());
}

template <typename F>
double timeMs(F work)
{
    const auto start = std::chrono::steady_clock::now();
    work();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

const PerfSample &perfSample()
{
    static PerfSample sample{0.0, 0.0, 0.0};
    static bool measured = false;
    if (!measured) {
        const std::size_t n = 100000;
        double sink = 0.0;
        sample.myMs = timeMs([&]() { sink += appendBenchMy(n, false); });
        sample.stdMs = timeMs([&]() { sink += appendBenchStd(n); });
        sample.reserveMs = timeMs([&]() { sink += appendBenchMy(n, true); });
        if (sink < 0.0) // 保留一个不可省略的依赖，避免整段被优化掉
            std::cerr << "";
        measured = true;
    }
    return sample;
}

/* 运行单个用例，返回完整输出（功能行 + PERF 行） */
std::string runCase(std::istream &fin)
{
    std::ostringstream out;

    std::string init;
    if (!(fin >> init))
        return out.str();

    MyString cur;
    if (init != "EMPTY")
        cur = MyString(init.c_str());

    int m = 0;
    if (!(fin >> m))
        return out.str();

    for (int i = 0; i < m; ++i) {
        std::string op;
        if (!(fin >> op))
            break;

        if (op == "PRINT") {
            out << (cur.empty() ? "EMPTY" : cur.c_str()) << '\n';
        } else if (op == "LENGTH") {
            out << cur.length() << '\n';
        } else if (op == "APPEND") {
            std::string t;
            fin >> t;
            cur.append(MyString(t.c_str()));
        } else if (op == "SUBSTR") {
            long long pos = 0;
            long long len = 0;
            fin >> pos >> len;
            if (pos < 0 || len < 0) {
                out << "ERROR\n";
                continue;
            }
            try {
                const MyString sub = cur.substr(static_cast<std::size_t>(pos),
                                                static_cast<std::size_t>(len));
                out << sub.c_str() << '\n';
            } catch (const std::out_of_range &) {
                out << "ERROR\n";
            }
        } else if (op == "FIND") {
            std::string t;
            fin >> t;
            const std::size_t pos = cur.find(MyString(t.c_str()));
            if (pos == MyString::npos)
                out << "-1\n";
            else
                out << pos << '\n';
        } else if (op == "REPLACE") {
            std::string oldStr;
            std::string newStr;
            fin >> oldStr >> newStr;
            cur.replace(MyString(oldStr.c_str()), MyString(newStr.c_str()));
        } else if (op == "COMPARE") {
            std::string t;
            fin >> t;
            out << cur.compare(MyString(t.c_str())) << '\n';
        } else if (op == "CLEAR") {
            cur.clear();
        }
        /* 未知操作静默忽略，保持程序健壮 */
    }

    const PerfSample &perf = perfSample();
    out << std::fixed << std::setprecision(3);
    out << "PERF my_string n=100000 ops=200000 time_ms=" << perf.myMs << '\n';
    out << "PERF std_string n=100000 ops=200000 time_ms=" << perf.stdMs << '\n';
    out << "PERF my_string_reserve n=100000 ops=200000 time_ms=" << perf.reserveMs << '\n';

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

/* 校验 PERF 行：名称须与期望一致，且包含 n= / ops= / time_ms= 三个字段 */
bool perfMatches(const std::string &actual, const std::string &expected)
{
    std::istringstream ae(actual);
    std::istringstream ee(expected);
    std::string tokenA;
    std::string tokenE;
    if (!(ae >> tokenA >> tokenA)) // 第一个 token 为 "PERF"
        return false;
    if (!(ee >> tokenE >> tokenE))
        return false;
    if (tokenA != tokenE)
        return false;
    return actual.find(" n=") != std::string::npos &&
           actual.find(" ops=") != std::string::npos &&
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