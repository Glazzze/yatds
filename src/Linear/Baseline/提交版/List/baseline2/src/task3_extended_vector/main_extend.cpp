/*
 * Baseline 任务 2 · 小任务 3：扩展可变数组的功能自测程序
 *
 * 对 6 个扩展功能逐一构造期望结果并断言：
 *   pop_back / resize / swap / assign / find / shrink_to_fit
 * 全部通过时输出统计信息；末尾追加扩展数组与 std::vector 的耗时对比 PERF 行。
 */

#include "vector.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

/* 已通过 / 失败的检查项数量 */
static int gPassed = 0;
static int gFailed = 0;

/* 断言辅助：比较两个整数序列 */
static void expect(const std::string &name, const std::vector<int> &actual,
                   const std::vector<int> &expected)
{
    if (actual == expected) {
        gPassed++;
    } else {
        gFailed++;
        std::cerr << "检查失败: " << name << '\n';
        std::cerr << "  期望: ";
        for (int x : expected)
            std::cerr << x << ' ';
        std::cerr << "\n  实际: ";
        for (int x : actual)
            std::cerr << x << ' ';
        std::cerr << '\n';
    }
}

/* 断言辅助：比较字符串序列（非平凡类型验证用） */
static void expect(const std::string &name, const std::vector<std::string> &actual,
                   const std::vector<std::string> &expected)
{
    if (actual == expected) {
        gPassed++;
    } else {
        gFailed++;
        std::cerr << "检查失败: " << name << '\n';
    }
}

/* 断言辅助：比较整型结果 */
static void expectInt(const std::string &name, long long actual, long long expected)
{
    if (actual == expected) {
        gPassed++;
    } else {
        gFailed++;
        std::cerr << "检查失败: " << name << "，期望 " << expected << "，实际 " << actual << '\n';
    }
}

/* 一次性完成全部扩展功能的自测 */
static void testExtendedFeatures()
{
    /* ---- pop_back：删除尾元素 ---- */
    {
        dsaac::Vector<int> v;
        for (int i = 1; i <= 4; i++)
            v.push_back(i); // 1 2 3 4
        v.pop_back();       // 1 2 3
        expect("pop_back 后内容", v.to_vector(), {1, 2, 3});
        expectInt("pop_back 后 size", static_cast<long long>(v.size()), 3);

        bool threw = false;
        dsaac::Vector<int> empty_vec;
        try {
            empty_vec.pop_back();
        } catch (const std::out_of_range &) {
            threw = true;
        }
        expectInt("空表 pop_back 抛出异常", threw ? 1 : 0, 1);
    }

    /* ---- resize：改变元素个数 ---- */
    {
        dsaac::Vector<int> v;
        for (int i = 1; i <= 3; i++)
            v.push_back(i); // 1 2 3
        v.resize(5, 9);     // 1 2 3 9 9
        expect("resize 扩大补值", v.to_vector(), {1, 2, 3, 9, 9});
        v.resize(2); // 1 2
        expect("resize 缩小截断", v.to_vector(), {1, 2});
        expectInt("resize 缩小后 size", static_cast<long long>(v.size()), 2);
    }

    /* ---- swap：交换内容 ---- */
    {
        dsaac::Vector<int> a;
        a.push_back(1);
        a.push_back(2); // a: 1 2
        dsaac::Vector<int> b;
        b.push_back(7);
        b.push_back(8);
        b.push_back(9); // b: 7 8 9
        a.swap(b);
        expect("swap 后 a 内容", a.to_vector(), {7, 8, 9});
        expect("swap 后 b 内容", b.to_vector(), {1, 2});
    }

    /* ---- assign：重新填充 ---- */
    {
        dsaac::Vector<int> v;
        v.push_back(1);
        v.assign(4, 5); // 5 5 5 5
        expect("assign 后内容", v.to_vector(), {5, 5, 5, 5});
        expectInt("assign 后 size", static_cast<long long>(v.size()), 4);
    }

    /* ---- find：查找下标 ---- */
    {
        dsaac::Vector<int> v;
        for (int x : {4, 6, 8, 6})
            v.push_back(x); // 4 6 8 6
        expectInt("find(6) 命中第一个", v.find(6), 1);
        expectInt("find(8) 命中", v.find(8), 2);
        expectInt("find(100) 未命中", v.find(100), -1);
    }

    /* ---- shrink_to_fit：容量收缩到长度 ---- */
    {
        dsaac::Vector<int> v;
        for (int i = 0; i < 10; i++)
            v.push_back(i); // 容量会增长到 >= 10
        expectInt("shrink 前容量不小于长度", static_cast<long long>(v.capacity()) >= 10 ? 1 : 0, 1);
        v.shrink_to_fit();
        expectInt("shrink 后容量等于长度", static_cast<long long>(v.capacity()),
                  static_cast<long long>(v.size()));
        expect("shrink 后内容不变", v.to_vector(), {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    }

    /* ---- 边界：收缩到空、空表扩容、容量收缩后复用 ---- */
    {
        dsaac::Vector<int> v;
        for (int i = 0; i < 5; i++)
            v.push_back(i);
        v.resize(0); // 截断到空
        expect("resize(0) 后为空", v.to_vector(), std::vector<int>{});
        expectInt("resize(0) 保留容量", static_cast<long long>(v.capacity()) >= 5 ? 1 : 0, 1);

        v.resize(2, 7); // 空表向上扩容补值
        expect("空表 resize(2,7) 补值", v.to_vector(), {7, 7});

        v.clear();
        v.shrink_to_fit(); // 空表收缩
        expectInt("空表 shrink 后容量归零", static_cast<long long>(v.capacity()), 0);
        expectInt("空表 shrink 后 size 为 0", static_cast<long long>(v.size()), 0);

        v.push_back(42); // 收缩到空后仍可复用
        expect("收缩到空后复用", v.to_vector(), {42});
    }

    /* ---- 非平凡类型：构造 / 析构严格配对（对 std::string 正确）---- */
    {
        dsaac::Vector<std::string> sv;
        sv.push_back("alpha");
        sv.push_back("beta");
        sv.push_back("gamma");
        sv.pop_back(); // 立即析构 "gamma"
        expectInt("string pop_back 后 size", static_cast<long long>(sv.size()), 2);

        sv.insert(0, "zero"); // zero alpha beta
        sv.erase(1);          // 删 "alpha" -> zero beta
        expect("string 混合操作后内容", sv.to_vector(), std::vector<std::string>{"zero", "beta"});
        expectInt("string front 为 zero", sv.front() == "zero" ? 1 : 0, 1);
        expectInt("string back 为 beta", sv.back() == "beta" ? 1 : 0, 1);

        sv.shrink_to_fit(); // 触发“析构旧元素 + 就地构造新元素”的搬移
        expect("string shrink 后内容不变", sv.to_vector(), std::vector<std::string>{"zero", "beta"});

        sv.resize(0); // 析构全部 string 元素
        expectInt("string resize(0) 后 size", static_cast<long long>(sv.size()), 0);
    }
}

/* 以毫秒为单位返回执行 fn 一次的耗时 */
template <typename Fn>
static double measureTimeMs(Fn &&fn)
{
    auto start = std::chrono::steady_clock::now();
    fn();
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;
    return diff.count();
}

/* 每项基准重复执行次数（取均值与总体标准差，报告统计离散度） */
static const int REPEAT = 24;

struct Stat {
    double mean;
    double sd;
};

/* 重复 REPEAT 次执行 fn，返回均值与总体标准差（毫秒） */
template <typename Fn>
static Stat benchRepeat(Fn &&fn)
{
    std::vector<double> samples;
    samples.reserve(REPEAT);
    for (int r = 0; r < REPEAT; r++)
        samples.push_back(measureTimeMs(fn));
    double mean = 0.0;
    for (double s : samples)
        mean += s;
    mean /= samples.size();
    double var = 0.0;
    for (double s : samples)
        var += (s - mean) * (s - mean);
    var /= samples.size();
    return Stat{mean, std::sqrt(var)};
}

/* 扩展数组基准操作：尾部追加 n 次 + 头部删除 n 次（覆盖 pop_back/erase 等新接口） */
static void runExtended(int n)
{
    dsaac::Vector<int> v;
    for (int i = 0; i < n; i++)
        v.push_back(i);
    while (!v.empty())
        v.pop_back();
}

/* std::vector 同规模基准操作，便于对照 */
static void runStdVector(int n)
{
    std::vector<int> v;
    for (int i = 0; i < n; i++)
        v.push_back(i);
    while (!v.empty())
        v.pop_back();
}

int main()
{
    std::ios::sync_with_stdio(false);

    /* 第一部分：扩展功能自测 */
    testExtendedFeatures();
    std::cout << "# 扩展功能自测：通过 " << gPassed << " 项，失败 " << gFailed << " 项\n";
    if (gFailed > 0)
        return 1;

    /* 第二部分：与 std::vector 的耗时对比（重复 REPEAT 次取均值与标准差） */
    const int sizes[] = {100000, 1000000};
    for (int n : sizes) {
        long long opsCount = 2LL * n; // 追加 n 次 + 删除 n 次
        Stat tOur = benchRepeat([n] { runExtended(n); });
        Stat tStd = benchRepeat([n] { runStdVector(n); });
        std::cout << "PERF vector_ext n=" << n << " ops=" << opsCount
                  << " time_ms=" << std::fixed << std::setprecision(6) << tOur.mean
                  << " sd=" << tOur.sd << '\n';
        std::cout << "PERF std_vector_ext n=" << n << " ops=" << opsCount
                  << " time_ms=" << tStd.mean << " sd=" << tStd.sd << '\n';
    }
    return 0;
}