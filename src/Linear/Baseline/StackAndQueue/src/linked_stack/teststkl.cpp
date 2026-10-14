/*
 * 链式栈测试程序：接口自检 + 命令式样例比对 + 性能统计。
 *
 * 按 README 规定的输入输出格式读取 inputs/ 中的样例，对每个操作
 * 驱动 dsaac::LinkedStack<T> 执行并产生输出，再与 outputs/ 中的
 * 标准输出逐用例比对，报告每个用例的 PASS / FAIL。
 *
 * 接口说明：LinkedStack<T> 只暴露 empty / push / pop / top 四个操作
 * （无 size、无遍历接口，也无 capacity/full——链表按需分配）。
 * 因此命令式驱动中：
 *   - SIZE 通过把元素逐个 pop 到 vector 计数，再倒回原栈实现；
 *   - PRINT 用同一倒栈过程天然得到"栈顶到栈底"的顺序。
 *
 * 输入格式（父级 README 统一规定）：
 *   第 1 行：capacity n（链式栈忽略 capacity，但仍需读取以保持格式一致）
 *   第 2 行：a1 ... an（从栈底到栈顶的初始元素；n = 0 时为空行）
 *   第 3 行：m（操作次数）
 *   接下来 m 行：op args...
 *
 * 操作：PUSH x / POP / TOP / SIZE / EMPTY / PRINT
 *
 * 输出约定：
 *   POP、TOP、SIZE、EMPTY、PRINT 逐行输出；
 *   空栈 POP/TOP 输出 ERROR（链表栈无满栈，PUSH 永不失败）；
 *   布尔值输出 true / false；空栈打印输出 EMPTY。
 */
#include "stackli.hpp"

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

/* ============================ 第一部分：接口自检 ============================ */

static int gPassed = 0;  // 自检通过计数
static int gFailed = 0;  // 自检失败计数

/* 断言辅助：比较两个整数并计数 */
static void expectInt(const std::string &name, long long actual, long long expected)
{
    if (actual == expected) {
        gPassed++;
    } else {
        gFailed++;
        std::cerr << "[FAIL] " << name << "：期望 " << expected << "，实际 " << actual << '\n';
    }
}

/* 断言辅助：比较两个布尔值并计数 */
static void expectBool(const std::string &name, bool actual, bool expected)
{
    expectInt(name, actual ? 1 : 0, expected ? 1 : 0);
}

/*
 * 接口自检：不依赖输入样例，直接验证类的行为契约。
 * 覆盖：LIFO 顺序、empty 边界、空栈 pop/top 下溢异常、
 * top 只读不改栈、深栈（无容量上限）、模板多类型实例化。
 */
template <typename T>
static void runInterfaceChecks(const std::string &typeName, T sampleValue)
{
    // 后进先出 + 基本状态
    {
        dsaac::LinkedStack<T> stk;
        expectBool(typeName + ": 初始为空", stk.empty(), true);
        stk.push(sampleValue);
        stk.push(sampleValue);
        expectBool(typeName + ": 入栈后非空", stk.empty(), false);
        expectInt(typeName + ": top 取到栈顶", stk.top() == sampleValue ? 1 : 0, 1);
        stk.pop();
        stk.pop();
        expectBool(typeName + ": 全部弹出后为空", stk.empty(), true);
    }
    // 空栈 pop 抛 out_of_range
    {
        dsaac::LinkedStack<T> stk;
        bool threw = false;
        try {
            stk.pop();
        } catch (const std::out_of_range &) {
            threw = true;
        } catch (...) {
            threw = false;
        }
        expectInt(typeName + ": 空栈 pop 抛 out_of_range", threw ? 1 : 0, 1);
    }
    // 空栈 top 抛 out_of_range
    {
        dsaac::LinkedStack<T> stk;
        bool threw = false;
        try {
            stk.top();
        } catch (const std::out_of_range &) {
            threw = true;
        } catch (...) {
            threw = false;
        }
        expectInt(typeName + ": 空栈 top 抛 out_of_range", threw ? 1 : 0, 1);
    }
    // LIFO 顺序（用可区分的三个值）
    {
        dsaac::LinkedStack<T> stk;
        stk.push(T(1));
        stk.push(T(2));
        stk.push(T(3));
        bool orderOk = (stk.top() == T(3));
        stk.pop();
        orderOk = orderOk && (stk.top() == T(2));
        stk.pop();
        orderOk = orderOk && (stk.top() == T(1));
        stk.pop();
        expectInt(typeName + ": 后进先出顺序", orderOk ? 1 : 0, 1);
    }
    // top 只读，不改变栈
    {
        dsaac::LinkedStack<T> stk;
        stk.push(sampleValue);
        bool stable = (stk.top() == sampleValue) && (stk.top() == sampleValue) && !stk.empty();
        expectInt(typeName + ": top 只读不改栈", stable ? 1 : 0, 1);
    }
    // 深栈：连续入栈大量元素（无容量上限），倒栈计数应精确
    {
        dsaac::LinkedStack<T> stk;
        const int N = 1000;
        for (int i = 0; i < N; ++i)
            stk.push(T(i));
        int count = 0;
        std::vector<T> popped;
        while (!stk.empty()) {
            popped.push_back(stk.top());
            stk.pop();
            count++;
        }
        bool deepOk = (count == N) && (popped.front() == T(N - 1)) && (popped.back() == T(0));
        expectInt(typeName + ": 深栈 1000 元素顺序正确", deepOk ? 1 : 0, 1);
    }
}

/* ============================ 第二部分：命令式驱动 ============================ */

/*
 * 倒栈：把 stk 的全部元素逐个弹出并记录（顺序恰为"栈顶→栈底"），
 * 再逆序压回恢复原状。返回弹出序列。受限接口下观察整栈的唯一途径，O(n)。
 */
static std::vector<int> drainRestore(dsaac::LinkedStack<int> &stk)
{
    std::vector<int> popped;
    while (!stk.empty()) {
        popped.push_back(stk.top());
        stk.pop();
    }
    for (auto it = popped.rbegin(); it != popped.rend(); ++it)
        stk.push(*it);
    return popped;
}

/*
 * 执行单个用例：读取输入流，返回程序输出内容（多行拼接）。
 * 异常策略与父级 README 一致：空栈访问输出 ERROR 而非中断——
 * 驱动程序先 empty() 预检查再操作，类本身的异常语义保留
 * （接口自检部分已验证）。链表栈无容量上限，PUSH 永不失败。
 */
static std::string run_case(std::istream &fin)
{
    long long capacity = 0;
    long long n = 0;
    if (!(fin >> capacity >> n))  // capacity 对链式栈无意义，仅读取以保持格式一致
        return std::string();

    dsaac::LinkedStack<int> stk;

    // 读取初始元素（从栈底到栈顶），链表栈无容量限制，全部入栈
    for (long long i = 0; i < n; ++i) {
        int x = 0;
        if (!(fin >> x))
            break;
        stk.push(x);
    }

    std::ostringstream out;
    long long m = 0;
    if (!(fin >> m))
        return out.str();
    for (long long i = 0; i < m; ++i) {
        std::string op;
        if (!(fin >> op))
            break;
        if (op == "PUSH") {
            int x = 0;
            if (!(fin >> x))
                break;
            stk.push(x);  // 链表栈无满栈概念，必成功
        } else if (op == "POP") {
            if (stk.empty()) {
                out << "ERROR\n";
            } else {
                out << stk.top() << '\n';
                stk.pop();
            }
        } else if (op == "TOP") {
            if (stk.empty()) {
                out << "ERROR\n";
            } else {
                out << stk.top() << '\n';
            }
        } else if (op == "SIZE") {
            out << drainRestore(stk).size() << '\n';
        } else if (op == "EMPTY") {
            out << (stk.empty() ? "true" : "false") << '\n';
        } else if (op == "PRINT") {
            std::vector<int> popped = drainRestore(stk);
            if (popped.empty()) {
                out << "EMPTY\n";
            } else {
                for (std::size_t j = 0; j < popped.size(); ++j) {
                    if (j > 0) out << ' ';
                    out << popped[j];
                }
                out << '\n';
            }
        }
        // 未知操作静默忽略，保持程序健壮
    }
    return out.str();
}

/* ============================ 第三部分：性能统计 ============================ */

/* 计时辅助：执行 lambda 并返回毫秒数 */
template <typename Fn>
static double measureTimeMs(Fn &&fn)
{
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

/*
 * 性能基准：连续 push 满 n 个元素再全部 pop。
 * 与 std::stack<int>（默认底层 deque）跑相同负载对比
 * （数组栈属 array_stack 组，不在本组程序中比对）。
 *
 * g_sink 为 volatile 汇，防止 -O2 判定循环无副作用而将其消除。
 */
static volatile long long g_sink = 0;

static void printPerfLines()
{
    const long long sizes[] = {100000, 1000000};
    const int REPEAT = 3;  // 每项重复 3 次取平均，减少抖动

    for (long long n : sizes) {
        double tLinked = 0.0;
        double tStd = 0.0;
        for (int r = 0; r < REPEAT; ++r) {
            tLinked += measureTimeMs([&n]() {
                dsaac::LinkedStack<int> stk;
                for (long long i = 0; i < n; ++i)
                    stk.push(static_cast<int>(i));
                long long sum = 0;
                while (!stk.empty()) {
                    sum += stk.top();
                    stk.pop();
                }
                g_sink += sum;
            });
            tStd += measureTimeMs([&n]() {
                std::stack<int> stk;
                for (long long i = 0; i < n; ++i)
                    stk.push(static_cast<int>(i));
                long long sum = 0;
                while (!stk.empty()) {
                    sum += stk.top();
                    stk.pop();
                }
                g_sink += sum;
            });
        }
        tLinked /= REPEAT;
        tStd /= REPEAT;
        std::cout << "PERF linked_stack n=" << n << " ops=" << n * 2
                  << " time_ms=" << std::fixed << std::setprecision(2) << tLinked << '\n';
        std::cout << "PERF std_stack n=" << n << " ops=" << n * 2
                  << " time_ms=" << std::fixed << std::setprecision(2) << tStd << '\n';
    }
}

/* ============================ 主程序 ============================ */

/* 读取整个文件内容；返回是否成功打开（缺失文件须与空输出区分） */
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

/* 枚举目录下的全部 .txt 文件名（不含路径） */
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
    std::sort(files.begin(), files.end());  // 保证按文件名顺序执行
    return files;
}

int main(int argc, char *argv[])
{
    // 第一部分：接口自检（无论何种用法都先跑，失败即退出）
    runInterfaceChecks<int>("int 链式栈", 42);
    runInterfaceChecks<double>("double 链式栈", 3.14);
    runInterfaceChecks<char>("char 链式栈", 'x');
    std::cerr << "# 接口自检：通过 " << gPassed << " 项，失败 " << gFailed << " 项\n";
    if (gFailed > 0)
        return 1;

    // 支持两种用法：
    //   ./test                       自动遍历 inputs/ 与 outputs/ 比对
    //   ./test inputs/case01.txt     单用例模式，直接打印程序输出
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

    const std::string input_dir = "inputs";
    const std::string output_dir = "outputs";
    std::vector<std::string> cases = list_case_files(input_dir);
    if (cases.empty()) {
        std::cerr << "inputs/ 目录中没有找到 .txt 样例\n";
        return 1;
    }

    int passed = 0;
    for (const std::string &name : cases) {
        std::ifstream fin(input_dir + "/" + name);
        if (!fin.is_open()) {
            std::cout << name << ": FAIL (无法打开输入文件)\n";
            continue;
        }
        std::string expected;
        if (!read_file(output_dir + "/" + name, expected)) {
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

    // 第三部分：性能统计（比对结束后追加，时间随机器变化不参与比对）
    printPerfLines();

    return passed == static_cast<int>(cases.size()) ? 0 : 1;
}