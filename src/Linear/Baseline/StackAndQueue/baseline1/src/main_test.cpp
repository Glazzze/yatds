/*
 * Baseline 任务 1 · 小任务 1/2 的提交测试程序（编译为 test）。
 *
 * 读取 inputs/ 中的样例，把同一初始元素与操作序列在【数组栈】与
 * 【链表栈】两种实现上重放，逐行交叉校验：任何一种实现出错都会被发现，
 * 比"分别测试"更强的验证方式。功能输出后追加三行性能统计
 * （array_stack / linked_stack / std_stack），对应小任务 2 的两实现对比。
 *
 * 数组栈为固定容量、链表栈本身无容量上限。为使两种实现的输出可比，
 * 驱动层统一以输入的 capacity 为上界，对两栈同样限流（链式栈的"无界"
 * 仅体现在类本身，不受此处驱动策略影响），从而交叉校验始终成立。
 *
 * 输入格式（父级 README 统一规定）：
 *   第 1 行：capacity n
 *   第 2 行：a1 .. an（从栈底到栈顶；n = 0 时为空行）
 *   第 3 行：m
 *   其后 m 行：op args...
 * 操作：PUSH x / POP / TOP / SIZE / EMPTY / PRINT
 *
 * 用法：./test                 自动遍历 inputs/ 与 outputs/ 比对
 *       ./test inputs/case01.txt  单用例模式（仅打印纯功能输出 + PERF）
 */
#include "../../src/array_stack/stackar.hpp"
#include "../../src/linked_stack/stackli.hpp"

#include <dirent.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

/* 容量上限：防御性钳制，避免超大容量触发未捕获异常或整数溢出 */
static const long long kMaxCapacity = 10000000;  // 1 千万

/* 交叉校验失败标志：任一用例两实现输出不一致时置位 */
static bool g_crossMismatch = false;

/* 把容量钳制到合法区间 */
static long long clampCapacity(long long capacity)
{
    if (capacity < 0)
        return 0;
    if (capacity > kMaxCapacity)
        return kMaxCapacity;
    return capacity;
}

/*
 * 在给定栈上执行一次操作，把结果追加到 out。
 * size 为驱动维护的元素计数（两栈接口一致：empty/push/pop/top），
 * cap 为统一容量上界；满栈 / 空栈边界在此统一判定，保证两实现行为一致。
 * SIZE / PRINT 用倒栈技巧（受限接口无 size()、无遍历）——逐个弹出到
 * vector（天然得到栈顶→栈底序），再逆序压回恢复原状。
 */
template <typename Stack>
static void applyOp(Stack &stk, long long &size, long long cap,
                    const std::string &op, int x, std::ostream &out)
{
    if (op == "PUSH") {
        if (size >= cap) {
            out << "ERROR\n";  // 满栈上溢
        } else {
            stk.push(x);
            size++;
        }
    } else if (op == "POP") {
        if (stk.empty()) {
            out << "ERROR\n";
        } else {
            out << stk.top() << '\n';
            stk.pop();
            size--;
        }
    } else if (op == "TOP") {
        if (stk.empty())
            out << "ERROR\n";
        else
            out << stk.top() << '\n';
    } else if (op == "SIZE") {
        std::vector<int> popped;
        while (!stk.empty()) {
            popped.push_back(stk.top());
            stk.pop();
        }
        out << popped.size() << '\n';
        for (auto it = popped.rbegin(); it != popped.rend(); ++it)
            stk.push(*it);
    } else if (op == "EMPTY") {
        out << (stk.empty() ? "true" : "false") << '\n';
    } else if (op == "PRINT") {
        std::vector<int> popped;
        while (!stk.empty()) {
            popped.push_back(stk.top());
            stk.pop();
        }
        if (popped.empty()) {
            out << "EMPTY\n";
        } else {
            for (std::size_t j = 0; j < popped.size(); ++j) {
                if (j > 0) out << ' ';
                out << popped[j];
            }
            out << '\n';
        }
        for (auto it = popped.rbegin(); it != popped.rend(); ++it)
            stk.push(*it);
    }
    // 未知操作静默忽略
}

/*
 * 执行单个用例：读入输入流，在数组栈与链表栈上重放同一序列，
 * 交叉校验后返回数组栈的输出（作为规范结果）。
 */
static std::string run_case(std::istream &fin)
{
    long long capacity = 0;
    long long n = 0;
    if (!(fin >> capacity >> n))
        return std::string();
    capacity = clampCapacity(capacity);

    dsaac::ArrayStack<int> arr(static_cast<std::size_t>(capacity));
    dsaac::LinkedStack<int> lnk;
    long long arrSize = 0;
    long long lnkSize = 0;

    std::ostringstream outArr;
    std::ostringstream outLnk;

    // 初始元素（从栈底到栈顶）
    for (long long i = 0; i < n; ++i) {
        int x = 0;
        if (!(fin >> x))
            break;
        applyOp(arr, arrSize, capacity, "PUSH", x, outArr);
        applyOp(lnk, lnkSize, capacity, "PUSH", x, outLnk);
    }

    long long m = 0;
    if (fin >> m) {
        for (long long i = 0; i < m; ++i) {
            std::string op;
            if (!(fin >> op))
                break;
            int x = 0;
            if (op == "PUSH" && !(fin >> x))
                break;
            applyOp(arr, arrSize, capacity, op, x, outArr);
            applyOp(lnk, lnkSize, capacity, op, x, outLnk);
        }
    }

    // 交叉校验：两种实现必须逐字一致
    if (outArr.str() != outLnk.str()) {
        g_crossMismatch = true;
        std::cerr << "内部错误：数组栈与链表栈输出不一致\n";
    }
    return outArr.str();
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

/* volatile 汇：防止 -O2 判定基准循环无副作用而将其消除 */
static volatile long long g_sink = 0;

/*
 * 小任务 2：数组栈 / 链表栈 / std::stack 三方对比，输出规范要求的三行
 * PERF。数组栈以容量 n 预留空间（等价于构造时 reserve），与链表栈、std 同负载。
 */
static void printPerfLines()
{
    const long long sizes[] = {100000, 1000000};
    const int REPEAT = 3;

    for (long long n : sizes) {
        double tArr = 0.0, tLnk = 0.0, tStd = 0.0;
        for (int r = 0; r < REPEAT; ++r) {
            tArr += measureTimeMs([&n]() {
                dsaac::ArrayStack<int> stk(static_cast<std::size_t>(n));
                for (long long i = 0; i < n; ++i) stk.push(static_cast<int>(i));
                long long s = 0;
                while (!stk.empty()) { s += stk.top(); stk.pop(); }
                g_sink += s;
            });
            tLnk += measureTimeMs([&n]() {
                dsaac::LinkedStack<int> stk;
                for (long long i = 0; i < n; ++i) stk.push(static_cast<int>(i));
                long long s = 0;
                while (!stk.empty()) { s += stk.top(); stk.pop(); }
                g_sink += s;
            });
            tStd += measureTimeMs([&n]() {
                std::stack<int> stk;
                for (long long i = 0; i < n; ++i) stk.push(static_cast<int>(i));
                long long s = 0;
                while (!stk.empty()) { s += stk.top(); stk.pop(); }
                g_sink += s;
            });
        }
        tArr /= REPEAT; tLnk /= REPEAT; tStd /= REPEAT;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "PERF array_stack n=" << n << " ops=" << n * 2 << " time_ms=" << tArr << '\n';
        std::cout << "PERF linked_stack n=" << n << " ops=" << n * 2 << " time_ms=" << tLnk << '\n';
        std::cout << "PERF std_stack n=" << n << " ops=" << n * 2 << " time_ms=" << tStd << '\n';
    }
}

/* ============================ 主程序 ============================ */

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
    if (argc > 1) {  // 单用例模式
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
        std::cout << "交叉校验失败：数组栈与链表栈输出不全部一致\n";
        return 1;
    }
    return passed == static_cast<int>(cases.size()) ? 0 : 1;
}