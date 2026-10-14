/*
 * 数组栈测试程序：接口自检 + 命令式样例比对 + 性能统计。
 *
 * 按 README 规定的输入输出格式读取 inputs/ 中的样例，对每个操作
 * 驱动 dsaac::ArrayStack<T> 执行并产生输出，再与 outputs/ 中的
 * 标准输出逐用例比对，报告每个用例的 PASS / FAIL。
 *
 * 接口说明：ArrayStack<T> 只暴露 empty / full / push / pop / top
 * 五个操作（无 size、无遍历接口）。因此命令式驱动中：
 *   - SIZE 通过把元素逐个 pop 到临时栈计数，再倒回原栈实现；
 *   - PRINT 用同一倒栈过程天然得到"栈顶到栈底"的顺序。
 * 这既是受限接口下的实现技巧，也体现了"栈只暴露栈顶操作"的
 * 适配器哲学：任何整体遍历都不得不付出 O(n) 的倒栈代价。
 *
 * 输入格式（父级 README 统一规定）：
 *   第 1 行：capacity n（数组栈容量与初始元素个数）
 *   第 2 行：a1 ... an（从栈底到栈顶的初始元素；n = 0 时为空行）
 *   第 3 行：m（操作次数）
 *   接下来 m 行：op args...
 *
 * 操作：PUSH x / POP / TOP / SIZE / EMPTY / PRINT
 *
 * 输出约定：
 *   POP、TOP、SIZE、EMPTY、PRINT 逐行输出；
 *   空栈 POP/TOP 输出 ERROR；
 *   满栈 PUSH 输出 ERROR（固定容量语义新增的边界）；
 *   布尔值输出 true / false；空栈打印输出 EMPTY。
 */
#include "stackar.hpp"

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

/* 容量上限：防御性钳制，避免超大容量触发未捕获异常或整数溢出 */
static const long long kMaxCapacity = 10000000;  // 1 千万

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
 * 覆盖：LIFO 顺序、empty/full 边界、满栈 push 上溢异常、
 * 空栈 pop/top 下溢异常、top 只读不改栈、弹出腾出容量可复用、
 * capacity=0 的退化栈、模板多类型实例化。
 */
template <typename T>
static void runInterfaceChecks(const std::string &typeName, T sampleValue)
{
    // 后进先出 + 基本状态
    {
        dsaac::ArrayStack<T> stk(3);
        expectBool(typeName + ": 初始为空", stk.empty(), true);
        expectBool(typeName + ": 未满", stk.full(), false);
        stk.push(sampleValue);
        stk.push(sampleValue);
        stk.push(sampleValue);
        expectBool(typeName + ": 三元素栈非空", stk.empty(), false);
        expectBool(typeName + ": 容量 3 装入 3 个即满", stk.full(), true);
        expectInt(typeName + ": top 取到栈顶", stk.top() == sampleValue ? 1 : 0, 1);
        stk.pop();
        stk.pop();
        stk.pop();
        expectBool(typeName + ": 全部弹出后为空", stk.empty(), true);
    }
    // 满栈 push 抛 overflow_error
    {
        dsaac::ArrayStack<T> stk(2);
        stk.push(sampleValue);
        stk.push(sampleValue);
        bool threw = false;
        try {
            stk.push(sampleValue);
        } catch (const std::overflow_error &) {
            threw = true;
        } catch (...) {
            threw = false;
        }
        expectInt(typeName + ": 满栈 push 抛 overflow_error", threw ? 1 : 0, 1);
    }
    // 空栈 pop 抛 out_of_range
    {
        dsaac::ArrayStack<T> stk(2);
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
        dsaac::ArrayStack<T> stk(2);
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
        dsaac::ArrayStack<T> stk(3);
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
    // 弹出腾出容量后可继续入栈（容量复用）
    {
        dsaac::ArrayStack<T> stk(2);
        stk.push(T(1));
        stk.push(T(2));
        stk.pop();
        stk.push(T(3));  // 腾出的空位应可复用，不抛异常
        expectInt(typeName + ": 弹出后容量可复用", stk.top() == T(3) ? 1 : 0, 1);
    }
    // capacity = 0 的退化栈：立即为满
    {
        dsaac::ArrayStack<T> stk(0);
        expectBool(typeName + ": 容量 0 立即满", stk.full(), true);
        bool threw = false;
        try {
            stk.push(sampleValue);
        } catch (const std::overflow_error &) {
            threw = true;
        } catch (...) {
            threw = false;
        }
        expectInt(typeName + ": 容量 0 的 push 上溢", threw ? 1 : 0, 1);
    }
}

/* ============================ 第二部分：命令式驱动 ============================ */

/*
 * 执行单个用例：读取输入流，返回程序输出内容（多行拼接）。
 * 异常策略与父级 README 一致：空栈访问输出 ERROR 而非中断——
 * 驱动程序先 empty() 预检查再操作，类本身的异常语义保留
 * （接口自检部分已验证）。满栈 PUSH 同样输出 ERROR。
 */
static std::string run_case(std::istream &fin)
{
    long long capacity = 0;
    long long n = 0;
    if (!(fin >> capacity >> n))
        return std::string();

    // 容量防御：钳制到合理上限，避免用超大容量构造 vector 时抛出未捕获的
    // length_error/bad_alloc 导致程序终止；负容量按 0 处理。
    if (capacity < 0)
        capacity = 0;
    if (capacity > kMaxCapacity)
        capacity = kMaxCapacity;

    dsaac::ArrayStack<int> stk(static_cast<std::size_t>(capacity));

    std::ostringstream out;

    // 读取初始元素（从栈底到栈顶）。固定容量语义下初始元素与 PUSH 受同样的
    // 容量限制：超出的部分逐个输出 ERROR（与满栈 PUSH 语义一致，不额外多打）
    for (long long i = 0; i < n; ++i) {
        int x = 0;
        if (!(fin >> x))
            break;
        if (stk.full())
            out << "ERROR\n";
        else
            stk.push(x);
    }

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
            if (stk.full()) {
                out << "ERROR\n";  // 满栈上溢
            } else {
                stk.push(x);
            }
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
            // 受限接口无 size()：把元素逐个弹出计数（顺序恰为栈顶→栈底），
            // 再逆序压回原栈恢复原状
            std::vector<int> popped;
            while (!stk.empty()) {
                popped.push_back(stk.top());
                stk.pop();
            }
            out << popped.size() << '\n';
            // 倒回原栈（popped 是栈顶→栈底序，逆序压回即恢复原状）
            for (auto it = popped.rbegin(); it != popped.rend(); ++it)
                stk.push(*it);
        } else if (op == "EMPTY") {
            out << (stk.empty() ? "true" : "false") << '\n';
        } else if (op == "PRINT") {
            // 倒栈打印：pop 出的顺序天然是"栈顶→栈底"，打印后倒回
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
 * 性能基准：连续 push 满 n 个元素再全部 pop，与 std::stack<int> 跑
 * 相同负载对比（链表栈属 linked_stack 组，不在本组程序中比对）。
 * 数组栈以构造容量 n 预留空间，基准中不会触发上溢。
 *
 * g_sink 为 volatile 汇：把基准中的求和结果写回，防止编译器在 -O2
 * 下判定循环无副作用而将其消除（否则 std 基准可能被优化成空循环，
 * 导致对比失真）。配合 Makefile 的 -flto，手写类也能跨 TU 内联。
 */
static volatile long long g_sink = 0;

static void printPerfLines()
{
    const long long sizes[] = {100000, 1000000};
    const int REPEAT = 3;  // 每项重复 3 次取平均，减少抖动

    for (long long n : sizes) {
        double tArray = 0.0;
        double tStd = 0.0;
        for (int r = 0; r < REPEAT; ++r) {
            tArray += measureTimeMs([&n]() {
                dsaac::ArrayStack<int> stk(static_cast<std::size_t>(n));
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
                std::stack<int> stk;  // std::stack 默认底层为 deque
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
        tArray /= REPEAT;
        tStd /= REPEAT;
        std::cout << "PERF array_stack n=" << n << " ops=" << n * 2
                  << " time_ms=" << std::fixed << std::setprecision(2) << tArray << '\n';
        std::cout << "PERF std_stack n=" << n << " ops=" << n * 2
                  << " time_ms=" << std::fixed << std::setprecision(2) << tStd << '\n';
    }
}

/* ============================ 主程序 ============================ */

/* 读取整个文件内容；返回是否成功打开。失败与"内容为空"必须区分，
 * 否则缺失的标准输出文件会被误判为通过。 */
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
    // 第一部分：接口自检（横幅走 stderr——stdout 必须只含纯结果，
    // 单用例模式下的输出会被逐字符比对）
    runInterfaceChecks<int>("int 数组栈", 42);
    runInterfaceChecks<double>("double 数组栈", 3.14);
    runInterfaceChecks<char>("char 数组栈", 'x');
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
