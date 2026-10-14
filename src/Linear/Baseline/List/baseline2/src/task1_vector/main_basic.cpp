/*
 * Baseline 任务 2 · 小任务 1：可变数组实现的功能测试 + 性能统计程序
 *
 * 输入输出格式遵循课程 README 的规定：
 *   第 1 行  capacity n         （初始容量 / 初始元素个数，要求 capacity >= n）
 *   第 2 行  a1 a2 ... an       （初始元素，n=0 时为空行）
 *   第 3 行  m                  （操作次数）
 *   接下来 m 行 op args...       （PUSH_BACK / INSERT / ERASE / GET / SET /
 *                                 SIZE / CAPACITY / EMPTY / PRINT / CLEAR）
 *
 * 输出约定：
 *   GET/SIZE/CAPACITY/EMPTY/PRINT 逐行输出；GET 越界输出 ERROR；
 *   空数组 PRINT 输出 EMPTY；布尔值输出 true/false；
 *   功能输出后追加 PERF 性能统计行。
 *
 * 两种用法：
 *   ./test                  自动遍历 inputs/，与 outputs/ 同名文件比对（功能行严格比对，
 *                           PERF 行只校验格式与名称，因运行时间随机器变化）
 *   ./test inputs/case01.txt 单用例模式，直接打印该样例的完整输出，便于调试
 *
 * 交叉校验：同一初始元素与操作序列同时作用于自实现 dsaac::Vector 与 std::vector，
 * 逐行比对功能输出（容量行因扩容策略不同而跳过），保证两种实现语义一致。
 */

#include "vector.hpp"
#include "../common/linked_list/linked_list.hpp"

#include <dirent.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

/* 交叉校验是否发现不一致（供 main 判定退出码） */
static bool gCrossCheckFailed = false;

/* ---------- 跨容器操作适配：自实现 Vector 与 std::vector 接口不同处 ---------- */

static void insertAt(dsaac::Vector<int> &c, std::size_t i, int x) { c.insert(i, x); }
static void eraseAt(dsaac::Vector<int> &c, std::size_t i) { c.erase(i); }
static void insertAt(std::vector<int> &c, std::size_t i, int x) { c.insert(c.begin() + i, x); }
static void eraseAt(std::vector<int> &c, std::size_t i) { c.erase(c.begin() + i); }

/*
 * 用初始元素与操作序列驱动容器，返回全部输出行；capMask 与输出行一一对应，
 * 标记该行是否来自 CAPACITY 操作（容量值依赖扩容策略，交叉校验时跳过）。
 */
template <typename Container>
static std::vector<std::string> runOperations(const std::vector<std::string> &ops,
                                              const std::vector<int> &initElems,
                                              std::size_t capacity,
                                              std::vector<bool> &capMask)
{
    Container c;
    c.reserve(capacity);
    std::vector<std::string> out;
    capMask.clear();

    auto emit = [&](const std::string &s, bool isCapacity) {
        out.push_back(s);
        capMask.push_back(isCapacity);
    };

    for (int x : initElems)
        c.push_back(x);

    for (const std::string &line : ops) {
        std::istringstream iss(line);
        std::string op;
        iss >> op;

        if (op == "PUSH_BACK") {
            int x;
            iss >> x;
            c.push_back(x);
        } else if (op == "INSERT") {
            long long i;
            int x;
            iss >> i >> x;
            if (i < 0 || i > static_cast<long long>(c.size()))
                emit("ERROR", false);
            else
                insertAt(c, static_cast<std::size_t>(i), x);
        } else if (op == "ERASE") {
            long long i;
            iss >> i;
            if (i < 0 || i >= static_cast<long long>(c.size()))
                emit("ERROR", false);
            else
                eraseAt(c, static_cast<std::size_t>(i));
        } else if (op == "GET") {
            long long i;
            iss >> i;
            if (i < 0 || i >= static_cast<long long>(c.size()))
                emit("ERROR", false);
            else
                emit(std::to_string(c[static_cast<std::size_t>(i)]), false);
        } else if (op == "SET") {
            long long i;
            int x;
            iss >> i >> x;
            if (i < 0 || i >= static_cast<long long>(c.size()))
                emit("ERROR", false);
            else
                c[static_cast<std::size_t>(i)] = x;
        } else if (op == "SIZE") {
            emit(std::to_string(c.size()), false);
        } else if (op == "CAPACITY") {
            emit(std::to_string(c.capacity()), true);
        } else if (op == "EMPTY") {
            emit(c.empty() ? "true" : "false", false);
        } else if (op == "PRINT") {
            if (c.empty()) {
                emit("EMPTY", false);
            } else {
                std::ostringstream oss;
                for (std::size_t i = 0; i < c.size(); i++) {
                    if (i > 0)
                        oss << ' ';
                    oss << c[i];
                }
                emit(oss.str(), false);
            }
        } else if (op == "CLEAR") {
            c.clear();
        }
        // 未知操作静默忽略，保持程序健壮
    }
    return out;
}

/* ---------- 性能基准 ---------- */

template <typename Fn>
static double measureTimeMs(Fn &&fn)
{
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;
    return diff.count();
}

static double benchVector(int n)
{
    return measureTimeMs([n] {
        dsaac::Vector<int> v;
        for (int i = 0; i < n; i++)
            v.push_back(i);
        long long sum = 0;
        for (int i = 0; i < n; i++)
            sum += v[static_cast<std::size_t>(i)];
        (void)sum;
    });
}

static double benchStdVector(int n)
{
    return measureTimeMs([n] {
        std::vector<int> v;
        for (int i = 0; i < n; i++)
            v.push_back(i);
        long long sum = 0;
        for (int i = 0; i < n; i++)
            sum += v[static_cast<std::size_t>(i)];
        (void)sum;
    });
}

static double benchLinkedList(int n)
{
    return measureTimeMs([n] {
        LinkedList<int> list;
        for (int i = 0; i < n; i++)
            list.push_back(i);
        long long sum = 0;
        for (const LinkedList<int>::Node *p = list.head_node(); p != nullptr; p = p->next)
            sum += p->val;
        (void)sum;
    });
}

/* 运行单个用例，返回完整输出（功能行 + PERF 行） */
static std::string runCase(std::istream &fin)
{
    std::ostringstream out;

    int capacity = 0, n = 0;
    if (!(fin >> capacity >> n))
        return out.str();

    std::vector<int> initElems;
    for (int i = 0; i < n; i++) {
        int x;
        if (!(fin >> x))
            break;
        initElems.push_back(x);
    }

    int m = 0;
    fin >> m;
    fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::vector<std::string> ops;
    ops.reserve(m);
    std::string line;
    for (int i = 0; i < m; i++) {
        if (std::getline(fin, line))
            ops.push_back(line);
    }

    /* 交叉校验：自实现 Vector 与 std::vector 的功能输出必须一致（容量行跳过） */
    std::vector<bool> capMask;
    std::vector<std::string> ourOut =
        runOperations<dsaac::Vector<int>>(ops, initElems, static_cast<std::size_t>(capacity), capMask);
    std::vector<bool> refMask;
    std::vector<std::string> refOut =
        runOperations<std::vector<int>>(ops, initElems, static_cast<std::size_t>(capacity), refMask);

    if (ourOut.size() != refOut.size()) {
        gCrossCheckFailed = true;
    } else {
        for (std::size_t i = 0; i < ourOut.size(); i++) {
            if (!capMask[i] && ourOut[i] != refOut[i]) {
                gCrossCheckFailed = true;
                break;
            }
        }
    }

    for (const std::string &s : ourOut)
        out << s << '\n';

    /* 性能统计：数据规模取本样例规模与基准规模中的较大者 */
    const int benchN = 100000;
    int perfN = (n + m > benchN) ? (n + m) : benchN;
    long long opsCount = 2LL * perfN;

    double tVector = benchVector(perfN);
    double tStdVector = benchStdVector(perfN);
    double tLinked = benchLinkedList(perfN);

    out << std::fixed << std::setprecision(2);
    out << "PERF vector_like n=" << perfN << " ops=" << opsCount << " time_ms=" << tVector << '\n';
    out << "PERF std_vector n=" << perfN << " ops=" << opsCount << " time_ms=" << tStdVector << '\n';
    out << "PERF linked_list n=" << perfN << " ops=" << opsCount << " time_ms=" << tLinked << '\n';

    return out.str();
}

/* 读取整个文件内容为字符串 */
static std::string readFile(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return std::string();
    std::ostringstream oss;
    oss << f.rdbuf();
    return oss.str();
}

/* 枚举目录下全部 .txt 文件名（不含路径） */
static std::vector<std::string> listCaseFiles(const std::string &dir)
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

/* 判断一行是否为合法的 PERF 统计行（格式：PERF name n=<数> ops=<数> time_ms=<数>） */
static bool isPerfLine(const std::string &line)
{
    return line.rfind("PERF ", 0) == 0;
}

/* 从完整输出中拆分出功能行与 PERF 行 */
static void splitOutput(const std::string &text, std::vector<std::string> &functional,
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

int main(int argc, char *argv[])
{
    std::ios::sync_with_stdio(false);

    const std::string inputDir = "inputs";
    const std::string outputDir = "outputs";
    const char *perfNames[] = {"vector_like", "std_vector", "linked_list"};

    /* 单用例模式：直接打印输出，便于调试 */
    if (argc > 1) {
        std::ifstream fin(argv[1]);
        if (!fin.is_open()) {
            std::cerr << "无法打开输入文件: " << argv[1] << '\n';
            return 1;
        }
        std::cout << runCase(fin);
        return gCrossCheckFailed ? 1 : 0;
    }

    /* 自动遍历模式：逐一比对 inputs/ 与 outputs/ */
    std::vector<std::string> cases = listCaseFiles(inputDir);
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
        std::string expected = readFile(outputDir + "/" + name);
        std::string actual = runCase(fin);

        std::vector<std::string> actFunc, actPerf;
        splitOutput(actual, actFunc, actPerf);

        /* 功能行逐字符比对 */
        std::ostringstream actFuncText;
        for (const std::string &s : actFunc)
            actFuncText << s << '\n';

        bool funcOk = (actFuncText.str() == expected);

        /* PERF 行：3 行且名称、格式正确 */
        bool perfOk = (actPerf.size() == 3);
        for (const char *pname : perfNames) {
            std::string prefix = std::string("PERF ") + pname + " ";
            bool found = false;
            for (const std::string &pl : actPerf) {
                if (pl.rfind(prefix, 0) == 0 && pl.find("n=") != std::string::npos &&
                    pl.find("ops=") != std::string::npos && pl.find("time_ms=") != std::string::npos)
                    found = true;
            }
            if (!found)
                perfOk = false;
        }

        if (funcOk && perfOk && !gCrossCheckFailed) {
            std::cout << name << ": PASS\n";
            passed++;
        } else {
            std::cout << name << ": FAIL\n";
            if (!funcOk) {
                std::cout << "--- 期望 ---\n" << expected;
                std::cout << "--- 实际 ---\n" << actFuncText.str();
            }
            if (!perfOk)
                std::cout << "  PERF 行缺失或格式不正确（需 3 行：vector_like / std_vector / linked_list）\n";
            if (gCrossCheckFailed)
                std::cout << "  交叉校验失败：自实现 Vector 与 std::vector 功能输出不一致\n";
        }
        gCrossCheckFailed = false;
    }

    std::cout << "共 " << cases.size() << " 个用例，通过 " << passed << " 个\n";
    return passed == static_cast<int>(cases.size()) ? 0 : 1;
}