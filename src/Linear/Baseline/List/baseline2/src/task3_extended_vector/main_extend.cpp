/*
 * Baseline 任务 2 · 小任务 3：扩展可变数组的功能自测程序
 *
 * 对 6 个扩展功能逐一构造期望结果并断言：
 *   pop_back / resize / swap / assign / find / shrink_to_fit
 * 全部通过时输出统计信息；末尾追加扩展数组与 std::vector 的耗时对比 PERF 行。
 */

#include "vector.hpp"

#include <chrono>
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
}

/* 以毫秒为单位返回执行 fn 一次的耗时 */
template <typename Fn>
static double measureTimeMs(Fn &&fn)
{
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start;
    return diff.count();
}

/* 扩展数组基准：尾部追加 n 次 + 头部删除 n 次（覆盖 pop_back/erase 等新接口） */
static double benchExtended(int n)
{
    return measureTimeMs([n] {
        dsaac::Vector<int> v;
        for (int i = 0; i < n; i++)
            v.push_back(i);
        while (!v.empty())
            v.pop_back();
    });
}

/* std::vector 同规模基准，便于对照 */
static double benchStdVector(int n)
{
    return measureTimeMs([n] {
        std::vector<int> v;
        for (int i = 0; i < n; i++)
            v.push_back(i);
        while (!v.empty())
            v.pop_back();
    });
}

int main()
{
    std::ios::sync_with_stdio(false);

    /* 第一部分：扩展功能自测 */
    testExtendedFeatures();
    std::cout << "# 扩展功能自测：通过 " << gPassed << " 项，失败 " << gFailed << " 项\n";
    if (gFailed > 0)
        return 1;

    /* 第二部分：与 std::vector 的耗时对比 */
    const int sizes[] = {100000, 1000000};
    for (int n : sizes) {
        long long opsCount = 2LL * n; // 追加 n 次 + 删除 n 次
        double tOur = benchExtended(n);
        double tStd = benchStdVector(n);
        std::cout << "PERF vector_ext n=" << n << " ops=" << opsCount
                  << " time_ms=" << std::fixed << std::setprecision(2) << tOur << '\n';
        std::cout << "PERF std_vector_ext n=" << n << " ops=" << opsCount
                  << " time_ms=" << tStd << '\n';
    }
    return 0;
}