/*
 * Baseline 任务 1 · 小任务 3：扩展栈功能自测 + 与 std::stack 性能对比。
 *
 * 对扩展数组栈与扩展链式栈各执行一组断言（clear / swap / search /
 * peek / reverse 五项扩展功能 + 基础 LIFO），随后与 std::stack<int>
 * 跑相同基准，输出 PERF 行。编译为 stack_extend。
 */
#include "extended_stack.hpp"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <stack>
#include <stdexcept>
#include <string>

static int gPassed = 0;
static int gFailed = 0;

static void expectInt(const std::string &name, long long actual, long long expected)
{
    if (actual == expected) {
        gPassed++;
    } else {
        gFailed++;
        std::cerr << "[FAIL] " << name << "：期望 " << expected << "，实际 " << actual << '\n';
    }
}

static void expectBool(const std::string &name, bool actual, bool expected)
{
    expectInt(name, actual ? 1 : 0, expected ? 1 : 0);
}

/* 构造适配：扩展数组栈需要容量参数，扩展链式栈忽略容量 */
static dsaac::ExtendedArrayStack<int> makeExt(dsaac::ExtendedArrayStack<int> *, std::size_t cap)
{
    return dsaac::ExtendedArrayStack<int>(cap);
}
static dsaac::ExtendedLinkedStack<int> makeExt(dsaac::ExtendedLinkedStack<int> *, std::size_t)
{
    return dsaac::ExtendedLinkedStack<int>();
}
template <typename S>
static S makeExt(std::size_t cap)
{
    return makeExt(static_cast<S *>(nullptr), cap);
}

/* 对一个扩展栈实现跑全部扩展功能断言 */
template <typename S>
static void testExtendedFeatures(const std::string &name)
{
    // clear
    {
        S s = makeExt<S>(10);
        s.push(1); s.push(2); s.push(3);
        expectInt(name + ": size 3", static_cast<long long>(s.size()), 3);
        s.clear();
        expectBool(name + ": clear 后为空", s.empty(), true);
    }
    // 基础 LIFO
    {
        S s = makeExt<S>(10);
        s.push(1); s.push(2); s.push(3);
        expectInt(name + ": top==3", s.top(), 3);
        s.pop();
        expectInt(name + ": pop 后 top==2", s.top(), 2);
    }
    // swap
    {
        S a = makeExt<S>(10);
        S b = makeExt<S>(10);
        a.push(1); a.push(2);
        b.push(9);
        a.swap(b);
        expectInt(name + ": swap 后 a.top==9", a.top(), 9);
        expectInt(name + ": swap 后 b.top==2", b.top(), 2);
    }
    // search
    {
        S s = makeExt<S>(10);
        s.push(10); s.push(20); s.push(30);  // 栈顶 30
        expectInt(name + ": search(30)==1", s.search(30), 1);
        expectInt(name + ": search(10)==3", s.search(10), 3);
        expectInt(name + ": search(99)==-1", s.search(99), -1);
    }
    // peek（含越界异常）
    {
        S s = makeExt<S>(10);
        s.push(1); s.push(2); s.push(3);  // 栈顶 3
        expectInt(name + ": peek(1)==3", s.peek(1), 3);
        expectInt(name + ": peek(2)==2", s.peek(2), 2);
        bool threw = false;
        try {
            s.peek(4);
        } catch (const std::out_of_range &) {
            threw = true;
        } catch (...) {
            threw = false;
        }
        expectInt(name + ": peek 越界抛 out_of_range", threw ? 1 : 0, 1);
    }
    // reverse
    {
        S s = makeExt<S>(10);
        s.push(1); s.push(2); s.push(3);  // 栈顶 3
        s.reverse();                      // 栈顶变 1
        expectInt(name + ": reverse 后 top==1", s.top(), 1);
        s.reverse();                      // 还原，栈顶 3
        expectInt(name + ": 两次 reverse 还原 top==3", s.top(), 3);
    }
}

/* ============================ 性能对比 ============================ */

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
        double tArr = 0.0, tLnk = 0.0, tStd = 0.0;
        for (int r = 0; r < REPEAT; ++r) {
            tArr += measureTimeMs([&n]() {
                dsaac::ExtendedArrayStack<int> stk(static_cast<std::size_t>(n));
                for (long long i = 0; i < n; ++i) stk.push(static_cast<int>(i));
                long long s = 0;
                while (!stk.empty()) { s += stk.top(); stk.pop(); }
                g_sink += s;
            });
            tLnk += measureTimeMs([&n]() {
                dsaac::ExtendedLinkedStack<int> stk;
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
        std::cout << "PERF array_stack_ext n=" << n << " ops=" << n * 2 << " time_ms=" << tArr << '\n';
        std::cout << "PERF linked_stack_ext n=" << n << " ops=" << n * 2 << " time_ms=" << tLnk << '\n';
        std::cout << "PERF std_stack n=" << n << " ops=" << n * 2 << " time_ms=" << tStd << '\n';
    }
}

int main()
{
    testExtendedFeatures<dsaac::ExtendedArrayStack<int>>("扩展数组栈");
    testExtendedFeatures<dsaac::ExtendedLinkedStack<int>>("扩展链式栈");
    std::cout << "# 扩展功能自测：通过 " << gPassed << " 项，失败 " << gFailed << " 项\n";

    printPerfLines();

    return gFailed > 0 ? 1 : 0;
}