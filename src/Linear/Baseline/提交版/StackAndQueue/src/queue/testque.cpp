/*
 * 循环队列测试程序：接口自检 + 命令式样例比对 + 性能统计。
 *
 * 按 README 规定的输入输出格式读取 inputs/ 中的样例，对每个操作
 * 驱动 dsaac::CircularQueue<T> 执行并产生输出，再与 outputs/ 中的
 * 标准输出逐用例比对，报告每个用例的 PASS / FAIL。
 *
 * 接口说明：CircularQueue<T> 只暴露 empty / full / push / pop / front
 * 五个操作（无 size、无遍历接口，front_/back_ 私有）。因此命令式驱动中：
 *   - SIZE 通过把元素逐个 pop 到 vector 计数，再倒回原队列实现；
 *   - PRINT 用同一倒队过程天然得到"队首到队尾"的顺序。
 * 这既是受限接口下的实现技巧，也体现了"队列只暴露队首/队尾操作"的
 * 适配器哲学：任何整体遍历都不得不付出 O(n) 的倒队代价。
 *
 * 输入格式（父级 README 统一规定）：
 *   第 1 行：capacity n（数组队列容量与初始元素个数）
 *   第 2 行：a1 ... an（从队首到队尾的初始元素；n = 0 时为空行）
 *   第 3 行：m（操作次数）
 *   接下来 m 行：op args...
 *
 * 操作：PUSH x / POP / FRONT / SIZE / EMPTY / PRINT
 *
 * 输出约定：
 *   POP、FRONT、SIZE、EMPTY、PRINT 逐行输出；
 *   空队列 POP/FRONT 输出 ERROR；
 *   满队列 PUSH 输出 ERROR（固定容量语义新增的边界）；
 *   布尔值输出 true / false；空队列打印输出 EMPTY。
 */
#include "queue.hpp"

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

/* ============================ 第一部分：接口自检 ============================ */

static int gPassed = 0;  // 自检通过计数
static int gFailed = 0;  // 自检失败计数

/* 容量上限：防御性钳制，避免超大容量触发未捕获异常或整数溢出 */
static const long long kMaxCapacity = 10000000;  // 1 千万

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
 * 覆盖：FIFO 顺序、empty/full 边界、满队列 push 上溢异常、
 * 空队列 pop/front 下溢异常、front 只读不出队、环形回绕、
 * 出队腾位可复用、capacity=0 的退化队列、模板多类型实例化。
 */
template <typename T>
static void runInterfaceChecks(const std::string &typeName, T sampleValue)
{
    // 先进先出 + 基本状态
    {
        dsaac::CircularQueue<T> q(3);
        expectBool(typeName + ": 初始为空", q.empty(), true);
        expectBool(typeName + ": 未满", q.full(), false);
        q.push(T(1));
        q.push(T(2));
        q.push(T(3));
        expectBool(typeName + ": 容量 3 装入 3 个即满", q.full(), true);
        expectBool(typeName + ": 三元素队列非空", q.empty(), false);
        expectInt(typeName + ": front 取到队首", q.front() == T(1) ? 1 : 0, 1);
        q.pop();
        expectInt(typeName + ": 出队后队首推进到第二元素(FIFO)", q.front() == T(2) ? 1 : 0, 1);
        q.pop();
        q.pop();
        expectBool(typeName + ": 全部出队后为空", q.empty(), true);
    }
    // 满队列 push 抛 overflow_error
    {
        dsaac::CircularQueue<T> q(2);
        q.push(T(1));
        q.push(T(2));
        bool threw = false;
        try {
            q.push(T(3));
        } catch (const std::overflow_error &) {
            threw = true;
        } catch (...) {
            threw = false;
        }
        expectInt(typeName + ": 满队列 push 抛 overflow_error", threw ? 1 : 0, 1);
    }
    // 空队列 pop 抛 out_of_range
    {
        dsaac::CircularQueue<T> q(2);
        bool threw = false;
        try {
            q.pop();
        } catch (const std::out_of_range &) {
            threw = true;
        } catch (...) {
            threw = false;
        }
        expectInt(typeName + ": 空队列 pop 抛 out_of_range", threw ? 1 : 0, 1);
    }
    // 空队列 front 抛 out_of_range
    {
        dsaac::CircularQueue<T> q(2);
        bool threw = false;
        try {
            q.front();
        } catch (const std::out_of_range &) {
            threw = true;
        } catch (...) {
            threw = false;
        }
        expectInt(typeName + ": 空队列 front 抛 out_of_range", threw ? 1 : 0, 1);
    }
    // front 只读，不出队
    {
        dsaac::CircularQueue<T> q(3);
        q.push(sampleValue);
        bool stable = (q.front() == sampleValue) && (q.front() == sampleValue) && !q.empty();
        expectInt(typeName + ": front 只读不出队", stable ? 1 : 0, 1);
    }
    // 环形回绕：小容量反复 push/pop 使 back_ 越过缓冲区末尾回绕，
    // 元素顺序仍应正确（cap=3 → buffer 4 槽，下标 0..3）
    {
        dsaac::CircularQueue<T> q(3);
        q.push(T(1));
        q.push(T(2));
        q.push(T(3));  // 满，back_=3
        q.pop();
        q.pop();       // front_=2
        q.push(T(4));  // back_ 3→0 回绕
        q.push(T(5));  // back_ 0→1 回绕
        // 此刻队列应为 [3,4,5]
        bool wrapOk = (q.front() == T(3));
        q.pop();
        wrapOk = wrapOk && (q.front() == T(4));
        q.pop();
        wrapOk = wrapOk && (q.front() == T(5));
        expectInt(typeName + ": 环形回绕后顺序正确", wrapOk ? 1 : 0, 1);
    }
    // 出队腾出容量后可继续入队（容量复用）
    {
        dsaac::CircularQueue<T> q(2);
        q.push(T(1));
        q.push(T(2));
        q.pop();           // 腾出一个空位
        q.push(T(3));      // 应可复用，不抛异常
        expectInt(typeName + ": 出队腾位后可再入队", q.front() == T(2) ? 1 : 0, 1);
    }
    // capacity = 0 的退化队列：立即同时为空且满，push 上溢
    {
        dsaac::CircularQueue<T> q(0);
        expectBool(typeName + ": 容量 0 立即为空", q.empty(), true);
        expectBool(typeName + ": 容量 0 立即为满", q.full(), true);
        bool threw = false;
        try {
            q.push(sampleValue);
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
 * 倒队：把 q 的全部元素逐个出队并记录（顺序恰为"队首→队尾"），
 * 再按原顺序逐个压回恢复原状（FIFO 序不变）。受限接口下观察整队的
 * 唯一途径，O(n)。
 */
static std::vector<int> drainRestore(dsaac::CircularQueue<int> &q)
{
    std::vector<int> popped;
    while (!q.empty()) {
        popped.push_back(q.front());
        q.pop();
    }
    for (int v : popped)
        q.push(v);
    return popped;
}

/*
 * 执行单个用例：读取输入流，返回程序输出内容（多行拼接）。
 * 异常策略与父级 README 一致：空队列访问输出 ERROR 而非中断——
 * 驱动程序先 empty() 预检查再操作，类本身的异常语义保留
 * （接口自检部分已验证）。满队列 PUSH 同样输出 ERROR。
 */
static std::string run_case(std::istream &fin)
{
    long long capacity = 0;
    long long n = 0;
    if (!(fin >> capacity >> n))
        return std::string();

    // 容量防御：环形缓冲区实现分配 capacity+1 槽，超大容量会触发未捕获的
    // length_error/bad_alloc，或使 capacity+1 整数溢出；故钳制上限，负值归零
    if (capacity < 0)
        capacity = 0;
    if (capacity > kMaxCapacity)
        capacity = kMaxCapacity;

    dsaac::CircularQueue<int> q(static_cast<std::size_t>(capacity));

    std::ostringstream out;

    // 读取初始元素（从队首到队尾）。固定容量语义下初始元素与 PUSH 受同样的
    // 容量限制：超出的部分逐个输出 ERROR（与满队列 PUSH 语义一致）
    for (long long i = 0; i < n; ++i) {
        int x = 0;
        if (!(fin >> x))
            break;
        if (q.full())
            out << "ERROR\n";
        else
            q.push(x);
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
            if (q.full()) {
                out << "ERROR\n";  // 满队列上溢
            } else {
                q.push(x);
            }
        } else if (op == "POP") {
            if (q.empty()) {
                out << "ERROR\n";
            } else {
                out << q.front() << '\n';
                q.pop();
            }
        } else if (op == "FRONT") {
            if (q.empty()) {
                out << "ERROR\n";
            } else {
                out << q.front() << '\n';
            }
        } else if (op == "SIZE") {
            out << drainRestore(q).size() << '\n';
        } else if (op == "EMPTY") {
            out << (q.empty() ? "true" : "false") << '\n';
        } else if (op == "PRINT") {
            std::vector<int> popped = drainRestore(q);
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
 * 与 std::queue<int>（默认底层 deque）跑相同负载对比
 * （栈属 stack 组，不在本组程序中比对）。
 *
 * g_sink 为 volatile 汇，防止 -O2 判定循环无副作用而将其消除。
 */
static volatile long long g_sink = 0;

static void printPerfLines()
{
    const long long sizes[] = {100000, 1000000};
    const int REPEAT = 3;  // 每项重复 3 次取平均，减少抖动

    for (long long n : sizes) {
        double tCirc = 0.0;
        double tStd = 0.0;
        for (int r = 0; r < REPEAT; ++r) {
            tCirc += measureTimeMs([&n]() {
                dsaac::CircularQueue<int> q(static_cast<std::size_t>(n));
                for (long long i = 0; i < n; ++i)
                    q.push(static_cast<int>(i));
                long long sum = 0;
                while (!q.empty()) {
                    sum += q.front();
                    q.pop();
                }
                g_sink += sum;
            });
            tStd += measureTimeMs([&n]() {
                std::queue<int> q;
                for (long long i = 0; i < n; ++i)
                    q.push(static_cast<int>(i));
                long long sum = 0;
                while (!q.empty()) {
                    sum += q.front();
                    q.pop();
                }
                g_sink += sum;
            });
        }
        tCirc /= REPEAT;
        tStd /= REPEAT;
        std::cout << "PERF circular_queue n=" << n << " ops=" << n * 2
                  << " time_ms=" << std::fixed << std::setprecision(2) << tCirc << '\n';
        std::cout << "PERF std_queue n=" << n << " ops=" << n * 2
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
    // 第一部分：接口自检（横幅走 stderr——stdout 必须只含纯结果）
    runInterfaceChecks<int>("int 循环队列", 42);
    runInterfaceChecks<double>("double 循环队列", 3.14);
    runInterfaceChecks<char>("char 循环队列", 'x');
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