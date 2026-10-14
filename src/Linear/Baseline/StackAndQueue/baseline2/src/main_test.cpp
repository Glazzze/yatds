/*
 * Baseline 任务 2 · 小任务 1/3 的提交测试程序（编译为 test）。
 *
 * 读取 inputs/ 中的样例，把同一序列在【循环队列】（给定 queue.hpp）与
 * 【链式队列】两种实现上重放并逐行交叉校验；功能输出后追加三行性能统计
 * （circular_queue / linked_queue / std_queue）。
 *
 * 输入格式（父级 README 统一规定）：
 *   第 1 行：capacity n
 *   第 2 行：a1 .. an（从队首到队尾；n = 0 时为空行）
 *   第 3 行：m
 *   其后 m 行：op args...
 * 操作：PUSH x / POP / FRONT / SIZE / EMPTY / PRINT
 * 用法：./test [inputs/caseXX.txt]
 */
#include "linked_queue.hpp"
#include "../../src/queue/queue.hpp"

#include <dirent.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

static const long long kMaxCapacity = 10000000;
static bool g_crossMismatch = false;

static long long clampCapacity(long long capacity)
{
    if (capacity < 0) return 0;
    if (capacity > kMaxCapacity) return kMaxCapacity;
    return capacity;
}

/* 在给定队列上执行一次操作，结果追加到 out。size 为驱动维护的元素计数，
 * cap 为统一容量上界；SIZE/PRINT 用倒队技巧（受限接口无 size()、无遍历）——
 * 逐个出队到 vector（天然得到队首→队尾序），再按同序压回恢复原状。 */
template <typename Q>
static void applyOp(Q &q, long long &size, long long cap,
                    const std::string &op, int x, std::ostream &out)
{
    if (op == "PUSH") {
        if (size >= cap) out << "ERROR\n";
        else { q.push(x); size++; }
    } else if (op == "POP") {
        if (q.empty()) out << "ERROR\n";
        else { out << q.front() << '\n'; q.pop(); size--; }
    } else if (op == "FRONT") {
        if (q.empty()) out << "ERROR\n";
        else out << q.front() << '\n';
    } else if (op == "SIZE") {
        std::vector<int> popped;
        while (!q.empty()) { popped.push_back(q.front()); q.pop(); }
        out << popped.size() << '\n';
        for (int v : popped) q.push(v);  // 同序压回，FIFO 不变
    } else if (op == "EMPTY") {
        out << (q.empty() ? "true" : "false") << '\n';
    } else if (op == "PRINT") {
        std::vector<int> popped;
        while (!q.empty()) { popped.push_back(q.front()); q.pop(); }
        if (popped.empty()) out << "EMPTY\n";
        else {
            for (std::size_t j = 0; j < popped.size(); ++j) {
                if (j > 0) out << ' ';
                out << popped[j];
            }
            out << '\n';
        }
        for (int v : popped) q.push(v);
    }
}

static std::string run_case(std::istream &fin)
{
    long long capacity = 0, n = 0;
    if (!(fin >> capacity >> n)) return std::string();
    capacity = clampCapacity(capacity);

    dsaac::CircularQueue<int> cq(static_cast<std::size_t>(capacity));
    dsaac::LinkedQueue<int> lq;
    long long cSize = 0, lSize = 0;
    std::ostringstream outC, outL;

    for (long long i = 0; i < n; ++i) {
        int x = 0;
        if (!(fin >> x)) break;
        applyOp(cq, cSize, capacity, "PUSH", x, outC);
        applyOp(lq, lSize, capacity, "PUSH", x, outL);
    }
    long long m = 0;
    if (fin >> m) {
        for (long long i = 0; i < m; ++i) {
            std::string op;
            if (!(fin >> op)) break;
            int x = 0;
            if (op == "PUSH" && !(fin >> x)) break;
            applyOp(cq, cSize, capacity, op, x, outC);
            applyOp(lq, lSize, capacity, op, x, outL);
        }
    }
    if (outC.str() != outL.str()) {
        g_crossMismatch = true;
        std::cerr << "内部错误：循环队列与链式队列输出不一致\n";
    }
    return outC.str();
}

/* ============================ 性能统计 ============================ */

template <typename Fn>
static double measureTimeMs(Fn &&fn)
{
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

static volatile long long g_sink = 0;

static void printPerfLines()
{
    const long long sizes[] = {100000, 1000000};
    const int REPEAT = 3;
    for (long long n : sizes) {
        double tC = 0.0, tL = 0.0, tS = 0.0;
        for (int r = 0; r < REPEAT; ++r) {
            tC += measureTimeMs([&n]() {
                dsaac::CircularQueue<int> q(static_cast<std::size_t>(n));
                for (long long i = 0; i < n; ++i) q.push(static_cast<int>(i));
                long long s = 0;
                while (!q.empty()) { s += q.front(); q.pop(); }
                g_sink += s;
            });
            tL += measureTimeMs([&n]() {
                dsaac::LinkedQueue<int> q;
                for (long long i = 0; i < n; ++i) q.push(static_cast<int>(i));
                long long s = 0;
                while (!q.empty()) { s += q.front(); q.pop(); }
                g_sink += s;
            });
            tS += measureTimeMs([&n]() {
                std::queue<int> q;
                for (long long i = 0; i < n; ++i) q.push(static_cast<int>(i));
                long long s = 0;
                while (!q.empty()) { s += q.front(); q.pop(); }
                g_sink += s;
            });
        }
        tC /= REPEAT; tL /= REPEAT; tS /= REPEAT;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "PERF circular_queue n=" << n << " ops=" << n * 2 << " time_ms=" << tC << '\n';
        std::cout << "PERF linked_queue n=" << n << " ops=" << n * 2 << " time_ms=" << tL << '\n';
        std::cout << "PERF std_queue n=" << n << " ops=" << n * 2 << " time_ms=" << tS << '\n';
    }
}

/* ============================ 主程序 ============================ */

static bool read_file(const std::string &path, std::string &out)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::ostringstream oss;
    oss << f.rdbuf();
    out = oss.str();
    return true;
}

static std::vector<std::string> list_case_files(const std::string &dir)
{
    std::vector<std::string> files;
    DIR *dp = opendir(dir.c_str());
    if (dp == nullptr) return files;
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

int main(int argc, char *argv[])
{
    if (argc > 1) {
        std::ifstream fin(argv[1]);
        if (!fin.is_open()) {
            std::cerr << "无法打开输入文件: " << argv[1] << '\n';
            return 1;
        }
        std::cout << run_case(fin);
        printPerfLines();
        return g_crossMismatch ? 1 : 0;
    }

    std::vector<std::string> cases = list_case_files("inputs");
    if (cases.empty()) {
        std::cerr << "inputs/ 目录中没有找到 .txt 样例\n";
        return 1;
    }

    int passed = 0;
    for (const std::string &name : cases) {
        std::ifstream fin("inputs/" + name);
        if (!fin.is_open()) {
            std::cout << name << ": FAIL (无法打开输入文件)\n";
            continue;
        }
        std::string expected;
        if (!read_file("outputs/" + name, expected)) {
            std::cout << name << ": MISSING (缺少标准输出文件)\n";
            continue;
        }
        std::string actual = run_case(fin);
        if (actual == expected) {
            std::cout << name << ": PASS\n";
            passed++;
        } else {
            std::cout << name << ": FAIL\n";
            std::cout << "--- 期望 ---\n" << expected;
            std::cout << "--- 实际 ---\n" << actual;
        }
    }
    std::cout << "共 " << cases.size() << " 个用例，通过 " << passed << " 个\n";

    printPerfLines();

    if (g_crossMismatch) {
        std::cout << "交叉校验失败：两实现输出不全部一致\n";
        return 1;
    }
    return passed == static_cast<int>(cases.size()) ? 0 : 1;
}