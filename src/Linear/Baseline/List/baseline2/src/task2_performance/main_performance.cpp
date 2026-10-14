/*
 * Baseline 任务 2 · 小任务 2：自实现可变数组与 std::vector 的性能评估程序
 *
 * 在多种典型场景下对比 dsaac::Vector 与 std::vector 的耗时：
 *   1. append        ：连续尾部追加 n 次（考察扩容策略开销）
 *   2. append_reserve：预留容量后尾部追加 n 次（考察 reserve 对扩容的影响）
 *   3. access        ：建表后随机访问 n 次（考察随机访问 O(1) 特性）
 *   4. insert_front  ：头部插入 m 次（每次 O(n) 搬移，考察最坏情况）
 *   5. erase_front   ：头部删除 m 次（同上）
 *
 * 每项测试重复 REPEAT 次取平均以降低抖动，输出 PERF 统计行
 * （含数据规模、操作次数、平均运行时间 time_ms）。
 */

/* 复用 小任务 1 的可变数组实现（相对路径） */
#include "../task1_vector/vector.hpp"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

/* 每项测试重复执行的次数（取平均以降低计时抖动） */
static const int REPEAT = 5;

/* ---------- 跨容器操作适配：自实现 Vector 与 std::vector 接口不同处 ---------- */

static void insertAt(dsaac::Vector<int> &c, std::size_t i, int x) { c.insert(i, x); }
static void eraseAt(dsaac::Vector<int> &c, std::size_t i) { c.erase(i); }
static void insertAt(std::vector<int> &c, std::size_t i, int x) { c.insert(c.begin() + i, x); }
static void eraseAt(std::vector<int> &c, std::size_t i) { c.erase(c.begin() + i); }

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

/* 场景 1：连续尾部追加 n 次 */
template <typename Vec>
static double benchAppend(int n)
{
    return measureTimeMs([n] {
        Vec v;
        for (int i = 0; i < n; i++)
            v.push_back(i);
    });
}

/* 场景 2：预留容量后尾部追加 n 次 */
template <typename Vec>
static double benchAppendReserve(int n)
{
    return measureTimeMs([n] {
        Vec v;
        v.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; i++)
            v.push_back(i);
    });
}

/* 场景 3：建表后随机访问 n 次（线性同余生成下标，避免纯顺序预取） */
template <typename Vec>
static double benchAccess(int n)
{
    return measureTimeMs([n] {
        Vec v;
        for (int i = 0; i < n; i++)
            v.push_back(i);
        unsigned int seed = 20260314u;
        long long sum = 0;
        for (int i = 0; i < n; i++) {
            seed = seed * 1103515245u + 12345u;
            std::size_t idx = seed % static_cast<unsigned int>(n);
            sum += v[idx];
        }
        (void)sum;
    });
}

/* 场景 4：头部插入 m 次（每次搬移 O(size)） */
template <typename Vec>
static double benchInsertFront(int m)
{
    return measureTimeMs([m] {
        Vec v;
        for (int i = 0; i < m; i++)
            insertAt(v, 0, i);
    });
}

/* 场景 5：头部删除 m 次（每次搬移 O(size)） */
template <typename Vec>
static double benchEraseFront(int m)
{
    return measureTimeMs([m] {
        Vec v;
        for (int i = 0; i < m; i++)
            v.push_back(i);
        while (!v.empty())
            eraseAt(v, 0);
    });
}

/* 对指定容器执行某场景并返回重复 REPEAT 次的平均耗时（毫秒） */
template <typename Vec>
static double runBench(const std::string &scenario, int n)
{
    double total = 0.0;
    for (int r = 0; r < REPEAT; r++) {
        if (scenario == "append")
            total += benchAppend<Vec>(n);
        else if (scenario == "append_reserve")
            total += benchAppendReserve<Vec>(n);
        else if (scenario == "access")
            total += benchAccess<Vec>(n);
        else if (scenario == "insert_front")
            total += benchInsertFront<Vec>(n);
        else if (scenario == "erase_front")
            total += benchEraseFront<Vec>(n);
    }
    return total / REPEAT;
}

/* 场景对应的操作次数 */
static long long scenarioOps(const std::string &scenario, int n)
{
    if (scenario == "access")
        return 2LL * n; // 建表 n 次 + 访问 n 次
    if (scenario == "erase_front")
        return 2LL * n; // 入表 n 次 + 删除 n 次
    return n;
}

int main()
{
    std::ios::sync_with_stdio(false);

    /* 两种数据规模：10 万与 100 万；头部操作因 O(n^2) 单独用小规模 */
    const int sizes[] = {100000, 1000000};
    const std::string scenarios[] = {"append", "append_reserve", "access", "insert_front", "erase_front"};

    /* 保存每次测量结果，供汇总比值复用（避免二次运行引入计时噪声） */
    struct Row {
        std::string scenario;
        int scale;
        long long ops;
        double our;
        double stdVec;
    };
    std::vector<Row> rows;

    for (int n : sizes) {
        /* 头部插入/删除为 O(n^2)，规模缩小 100 倍以保证可运行 */
        int frontN = n / 100;
        std::cout << "# 数据规模 n=" << n << "（头部操作规模 m=" << frontN
                  << "），每项重复 " << REPEAT << " 次取平均\n";

        for (const std::string &scenario : scenarios) {
            bool isFront = (scenario == "insert_front" || scenario == "erase_front");
            int scale = isFront ? frontN : n;
            long long opsCount = scenarioOps(scenario, scale);

            double tOur = runBench<dsaac::Vector<int>>(scenario, scale);
            double tStd = runBench<std::vector<int>>(scenario, scale);
            rows.push_back({scenario, scale, opsCount, tOur, tStd});

            std::cout << "PERF vector_" << scenario << " n=" << scale << " ops=" << opsCount
                      << " time_ms=" << std::fixed << std::setprecision(3) << tOur << '\n';
            std::cout << "PERF std_vector_" << scenario << " n=" << scale << " ops=" << opsCount
                      << " time_ms=" << tStd << '\n';
        }
        std::cout << '\n';
    }

    /* 汇总：输出各场景耗时比值（自实现 / std::vector），便于报告引用 */
    std::cout << "# 汇总：time_ms 比值 = dsaac::Vector 耗时 / std::vector 耗时（>1 表示自实现更慢）\n";
    for (const Row &r : rows) {
        std::cout << "PERF ratio_" << r.scenario << " n=" << r.scale
                  << " time_ms=" << std::fixed << std::setprecision(3)
                  << (r.stdVec > 0 ? r.our / r.stdVec : 0.0) << '\n';
    }
    return 0;
}