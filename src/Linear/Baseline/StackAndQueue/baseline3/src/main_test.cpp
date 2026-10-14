/*
 * Baseline 任务 3 · 栈和队列的综合应用（编译为 test）。
 *
 * 支持三种场景（父级 README 规定）：
 *   STACK_BY_QUEUE  用队列实现栈（PUSH/POP/TOP/PRINT）
 *   QUEUE_BY_STACK  用栈实现队列（PUSH/POP/FRONT/PRINT）
 *   CONVERT         栈与队列互转（STACK_TO_QUEUE / QUEUE_TO_STACK）
 * 每个场景先输出 CASE t；空结构访问输出 ERROR，空打印输出 EMPTY。
 * 功能输出后追加三行性能统计：stack_by_queue / queue_by_stack / vector_based。
 *
 * 用法：./test [inputs/caseXX.txt]
 */
#include "stack_by_queue.hpp"
#include "queue_by_stack.hpp"
#include "vector_containers.hpp"

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

/* ---------- 遍历辅助：受限接口下打印／恢复 ---------- */

/* 打印栈（栈顶→栈底），随后恢复原状 */
template <typename S>
static void printStack(S &stk, std::ostream &out)
{
    std::vector<int> v;
    while (!stk.empty()) { v.push_back(stk.top()); stk.pop(); }
    if (v.empty()) {
        out << "EMPTY\n";
    } else {
        for (std::size_t j = 0; j < v.size(); ++j) {
            if (j > 0) out << ' ';
            out << v[j];
        }
        out << '\n';
    }
    for (auto it = v.rbegin(); it != v.rend(); ++it)
        stk.push(*it);
}

/* 打印队列（队首→队尾），随后按同序恢复 */
template <typename Q>
static void printQueue(Q &q, std::ostream &out)
{
    std::vector<int> v;
    while (!q.empty()) { v.push_back(q.front()); q.pop(); }
    if (v.empty()) {
        out << "EMPTY\n";
    } else {
        for (std::size_t j = 0; j < v.size(); ++j) {
            if (j > 0) out << ' ';
            out << v[j];
        }
        out << '\n';
    }
    for (int x : v) q.push(x);
}

/* ---------- 单用例（含 T 个场景）---------- */

static std::string run_case(std::istream &fin)
{
    std::ostringstream out;
    int T = 0;
    if (!(fin >> T))
        return out.str();

    for (int t = 1; t <= T; ++t) {
        std::string type;
        if (!(fin >> type))
            break;
        out << "CASE " << t << '\n';

        if (type == "STACK_BY_QUEUE") {
            long long n = 0;
            fin >> n;
            dsaac::StackByQueue<int> stk;
            for (long long i = 0; i < n; ++i) {
                int x = 0;
                if (!(fin >> x)) break;
                stk.push(x);
            }
            long long m = 0;
            fin >> m;
            for (long long i = 0; i < m; ++i) {
                std::string op;
                if (!(fin >> op)) break;
                if (op == "PUSH") {
                    int x = 0;
                    if (!(fin >> x)) break;
                    stk.push(x);
                } else if (op == "POP") {
                    if (stk.empty()) out << "ERROR\n";
                    else { out << stk.top() << '\n'; stk.pop(); }
                } else if (op == "TOP") {
                    if (stk.empty()) out << "ERROR\n";
                    else out << stk.top() << '\n';
                } else if (op == "PRINT") {
                    printStack(stk, out);
                }
            }
        } else if (type == "QUEUE_BY_STACK") {
            long long n = 0;
            fin >> n;
            dsaac::QueueByStack<int> q;
            for (long long i = 0; i < n; ++i) {
                int x = 0;
                if (!(fin >> x)) break;
                q.push(x);
            }
            long long m = 0;
            fin >> m;
            for (long long i = 0; i < m; ++i) {
                std::string op;
                if (!(fin >> op)) break;
                if (op == "PUSH") {
                    int x = 0;
                    if (!(fin >> x)) break;
                    q.push(x);
                } else if (op == "POP") {
                    if (q.empty()) out << "ERROR\n";
                    else { out << q.front() << '\n'; q.pop(); }
                } else if (op == "FRONT") {
                    if (q.empty()) out << "ERROR\n";
                    else out << q.front() << '\n';
                } else if (op == "PRINT") {
                    printQueue(q, out);
                }
            }
        } else if (type == "CONVERT") {
            long long n = 0;
            fin >> n;
            std::vector<int> seq;
            for (long long i = 0; i < n; ++i) {
                int x = 0;
                if (!(fin >> x)) break;
                seq.push_back(x);
            }
            std::string dir;
            fin >> dir;
            if (dir == "STACK_TO_QUEUE") {
                dsaac::VectorStack<int> s;
                for (int v : seq) s.push(v);
                dsaac::VectorQueue<int> q = s.to_queue();
                printQueue(q, out);  // 输出队首→队尾
            } else {  // QUEUE_TO_STACK
                dsaac::VectorQueue<int> q;
                for (int v : seq) q.push(v);
                dsaac::VectorStack<int> s = q.to_stack();
                printStack(s, out);  // 输出栈顶→栈底
            }
        }
    }
    return out.str();
}

/* ---------- 性能统计 ---------- */

template <typename Fn>
static double measureTimeMs(Fn &&fn)
{
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

static volatile long long g_sink = 0;

/*
 * 四方对比（规模取小值，因 stack_by_queue 的 pop 为 O(n)、n 个 pop 合计 O(n²)）：
 *   stack_by_queue  队列实现栈
 *   queue_by_stack  栈实现队列
 *   vector_based    vector 实现队列（queue_by_stack 的对照基准）
 *   vector_stack    vector 实现栈（stack_by_queue 的对照基准）
 */
static void printPerfLines()
{
    const long long sizes[] = {1000, 5000};
    const int REPEAT = 3;
    for (long long n : sizes) {
        double tSq = 0.0, tQs = 0.0, tVq = 0.0, tVs = 0.0;
        for (int r = 0; r < REPEAT; ++r) {
            tSq += measureTimeMs([&n]() {
                dsaac::StackByQueue<int> s;
                for (long long i = 0; i < n; ++i) s.push(static_cast<int>(i));
                long long acc = 0;
                while (!s.empty()) { acc += s.top(); s.pop(); }
                g_sink += acc;
            });
            tQs += measureTimeMs([&n]() {
                dsaac::QueueByStack<int> q;
                for (long long i = 0; i < n; ++i) q.push(static_cast<int>(i));
                long long acc = 0;
                while (!q.empty()) { acc += q.front(); q.pop(); }
                g_sink += acc;
            });
            tVq += measureTimeMs([&n]() {
                dsaac::VectorQueue<int> q;
                for (long long i = 0; i < n; ++i) q.push(static_cast<int>(i));
                long long acc = 0;
                while (!q.empty()) { acc += q.front(); q.pop(); }
                g_sink += acc;
            });
            tVs += measureTimeMs([&n]() {
                dsaac::VectorStack<int> s;
                for (long long i = 0; i < n; ++i) s.push(static_cast<int>(i));
                long long acc = 0;
                while (!s.empty()) { acc += s.top(); s.pop(); }
                g_sink += acc;
            });
        }
        tSq /= REPEAT; tQs /= REPEAT; tVq /= REPEAT; tVs /= REPEAT;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "PERF stack_by_queue n=" << n << " ops=" << n * 2 << " time_ms=" << tSq << '\n';
        std::cout << "PERF queue_by_stack n=" << n << " ops=" << n * 2 << " time_ms=" << tQs << '\n';
        std::cout << "PERF vector_based n=" << n << " ops=" << n * 2 << " time_ms=" << tVq << '\n';
        std::cout << "PERF vector_stack n=" << n << " ops=" << n * 2 << " time_ms=" << tVs << '\n';
    }
}

/* ---------- 主程序 ---------- */

static bool read_file(const std::string &path, std::string &out)
{
    std::ifstream f(path);
    if (!f.is_open())
        return false;
    std::ostringstream oss;
    oss << f.rdbuf();
    out = oss.str();
    return true;
}

static std::vector<std::string> list_case_files(const std::string &dir)
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
        return 0;
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

    return passed == static_cast<int>(cases.size()) ? 0 : 1;
}