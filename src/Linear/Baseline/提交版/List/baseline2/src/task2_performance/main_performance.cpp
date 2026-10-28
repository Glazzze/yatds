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
 * 每项测试重复 REPEAT 次，报告**均值与总体标准差**以体现计时离散度，
 * 输出 PERF 统计行（含数据规模、操作次数、平均运行时间 time_ms 与标准差 sd）；
 * 另输出：复杂度增长实测（growth 行）与 Welch 两样本 t 检验（STAT 行，含 95% 置信区间）。
 */

/* 复用 小任务 1 的可变数组实现（相对路径） */
#include "../task1_vector/vector.hpp"
#include "../common/stats/stats.hpp"
#include "../common/stats/benchmark.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

/* 每项测试重复执行的次数（取均值与标准差，报告统计离散度） */
static const int REPEAT = 24;

/* 一次场景的统计结果：均值与总体标准差（毫秒） */
struct Stat {
    double mean;
    double sd;
};

/* ---------- 跨容器操作适配：自实现 Vector 与 std::vector 接口不同处 ---------- */

static void insertAt(dsaac::Vector<int> &c, std::size_t i, int x) { c.insert(i, x); }
static void eraseAt(dsaac::Vector<int> &c, std::size_t i) { c.erase(i); }
static void insertAt(std::vector<int> &c, std::size_t i, int x) { c.insert(c.begin() + i, x); }
static void eraseAt(std::vector<int> &c, std::size_t i) { c.erase(c.begin() + i); }


/* 场景 1：连续尾部追加 n 次 */
template <typename Vec>
static double benchAppend(int n)
{
    Vec v;
    return dsaac::benchmark::measure([&] {
        for (int i = 0; i < n; ++i) v.push_back(i);
        dsaac::benchmark::escape(&v);
    });
}

/* 场景 2：预留容量后尾部追加 n 次 */
template <typename Vec>
static double benchAppendReserve(int n)
{
    Vec v;
    v.reserve(static_cast<std::size_t>(n)); // 预留容量不计入追加计时。
    return dsaac::benchmark::measure([&] {
        for (int i = 0; i < n; ++i) v.push_back(i);
        dsaac::benchmark::escape(&v);
    });
}

/* 场景 3：建表后随机访问 n 次（线性同余生成下标，避免纯顺序预取） */
template <typename Vec>
static double benchAccess(int n)
{
    Vec v;
    for (int i = 0; i < n; ++i) v.push_back(i);
    std::vector<std::size_t> indices;
    unsigned int seed = 20260314u;
    long long expected = 0;
    for (int i = 0; i < n; ++i) {
        seed = seed * 1103515245u + 12345u;
        indices.push_back(seed % static_cast<unsigned int>(n));
        expected += static_cast<long long>(indices.back());
    }
    long long sum = 0;
    double elapsed = dsaac::benchmark::measure([&] {
        for (auto index : indices) sum += v[index];
        dsaac::benchmark::consume(sum);
    });
    if (sum != expected) throw std::runtime_error("访问基准校验和错误");
    return elapsed;
}

/* 场景 4：头部插入 m 次（每次搬移 O(size)） */
template <typename Vec>
static double benchInsertFront(int m)
{
    Vec v;
    return dsaac::benchmark::measure([&] {
        for (int i = 0; i < m; ++i) {
            insertAt(v, 0, i);
            dsaac::benchmark::escape(&v);
        }
    });
}

/* 场景 5：头部删除 m 次（每次搬移 O(size)） */
template <typename Vec>
static double benchEraseFront(int m)
{
    Vec v;
    for (int i = 0; i < m; ++i) v.push_back(i);
    return dsaac::benchmark::measure([&] {
        while (!v.empty()) {
            eraseAt(v, 0);
            dsaac::benchmark::escape(&v);
        }
    });
}

/* 相同场景先预热，再交替两种实现的测量顺序，减轻先后次序偏差。 */
template <typename Vec>
static double measureScenario(const std::string& scenario, int n)
{
    if (scenario == "append") return benchAppend<Vec>(n);
    if (scenario == "append_reserve") return benchAppendReserve<Vec>(n);
    if (scenario == "access") return benchAccess<Vec>(n);
    if (scenario == "insert_front") return benchInsertFront<Vec>(n);
    if (scenario == "erase_front") return benchEraseFront<Vec>(n);
    throw std::invalid_argument("未知基准场景");
}
template <typename Vec>
static std::vector<double> runSamples(const std::string& scenario, int n)
{
    measureScenario<Vec>(scenario, n);
    std::vector<double> samples;
    for (int i = 0; i < REPEAT; ++i) samples.push_back(measureScenario<Vec>(scenario, n));
    return samples;
}
static std::pair<std::vector<double>, std::vector<double>> runPairedSamples(const std::string& scenario, int n)
{
    measureScenario<dsaac::Vector<int>>(scenario, n);
    measureScenario<std::vector<int>>(scenario, n);
    std::pair<std::vector<double>, std::vector<double>> result;
    for (int i = 0; i < REPEAT; ++i) {
        if (i % 2 == 0) {
            result.first.push_back(measureScenario<dsaac::Vector<int>>(scenario, n));
            result.second.push_back(measureScenario<std::vector<int>>(scenario, n));
        } else {
            result.second.push_back(measureScenario<std::vector<int>>(scenario, n));
            result.first.push_back(measureScenario<dsaac::Vector<int>>(scenario, n));
        }
    }
    return result;
}

/* 由原始样本得到均值与总体标准差 */
static Stat summarize(const std::vector<double> &samples)
{
    return Stat{dsaac::mean(samples), dsaac::sdPop(samples)};
}

/* 场景对应的操作次数 */
static long long scenarioOps(const std::string &scenario, int n)
{
    if (scenario == "access")
        return n; // 仅计访问；建表和下标生成不计时
    if (scenario == "erase_front")
        return n; // 仅计删除；建表不计时
    return n;
}

int main()
{
    std::ios::sync_with_stdio(false);

    /* 两种数据规模：10 万与 100 万；头部操作因 O(n^2) 单独用小规模 */
    const int sizes[] = {100000, 1000000};
    const std::string scenarios[] = {"append", "append_reserve", "access", "insert_front", "erase_front"};

    /* 保存每次测量结果（原始样本），供汇总比值与显著性检验复用 */
    struct Row {
        std::string scenario;
        int scale;
        long long ops;
        std::vector<double> our;
        std::vector<double> stdVec;
    };
    std::vector<Row> rows;

    for (int n : sizes) {
        /* 头部插入/删除为 O(n^2)，规模缩小 100 倍以保证可运行 */
        int frontN = n / 100;
        std::cout << "# 数据规模 n=" << n << "（头部操作规模 m=" << frontN
                  << "），每项重复 " << REPEAT << " 次，报告均值与标准差\n";

        for (const std::string &scenario : scenarios) {
            bool isFront = (scenario == "insert_front" || scenario == "erase_front");
            int scale = isFront ? frontN : n;
            long long opsCount = scenarioOps(scenario, scale);

            auto samples = runPairedSamples(scenario, scale);
            const auto& sOur = samples.first;
            const auto& sStd = samples.second;
            Stat tOur = summarize(sOur), tStd = summarize(sStd);
            rows.push_back({scenario, scale, opsCount, sOur, sStd});

            std::cout << "PERF vector_" << scenario << " n=" << scale << " ops=" << opsCount
                      << " time_ms=" << std::fixed << std::setprecision(3) << tOur.mean
                      << " sd=" << tOur.sd << '\n';
            std::cout << "PERF std_vector_" << scenario << " n=" << scale << " ops=" << opsCount
                      << " time_ms=" << tStd.mean << " sd=" << tStd.sd << '\n';
        }
        std::cout << '\n';
    }

    /* 复杂度实测：固定场景、逐级放大规模，观察耗时随 n 的增长（可佐证复杂度而非仅看绝对耗时）
       append_reserve 消除了扩容搬移，只留每次追加的均摊常数成本，期望随 n 近线性；
       insert_front 每次搬移整段，期望随 m 近平方。 */
    std::cout << "# 复杂度实测（dsaac::Vector）：append_reserve 期望随 n 近线性，insert_front 期望随 m 近平方\n";
    for (int n : {250000, 500000, 1000000}) {
        double t = dsaac::mean(runSamples<dsaac::Vector<int>>("append_reserve", n));
        std::cout << "PERF growth_append_reserve n=" << n << " time_ms="
                  << std::fixed << std::setprecision(3) << t << '\n';
    }
    for (int m : {2500, 5000, 10000}) {
        double t = dsaac::mean(runSamples<dsaac::Vector<int>>("insert_front", m));
        std::cout << "PERF growth_insert_front n=" << m << " time_ms="
                  << std::fixed << std::setprecision(3) << t << '\n';
    }
    std::cout << '\n';

    /* 汇总：输出各场景耗时比值（自实现 / std::vector），便于报告引用 */
    std::cout << "# 汇总：time_ms 比值 = dsaac::Vector 耗时 / std::vector 耗时（>1 表示自实现更慢）\n";
    for (const Row &r : rows) {
        double mo = dsaac::mean(r.our), ms = dsaac::mean(r.stdVec);
        std::cout << "PERF ratio_" << r.scenario << " n=" << r.scale
                  << " time_ms=" << std::fixed << std::setprecision(3)
                  << (ms > 0 ? mo / ms : 0.0) << '\n';
    }

    /* 统计显著性：Welch 两样本 t 检验（H0：两种实现均值相等）+ 均值差的 95% 置信区间 */
    std::cout << "# 统计显著性：Welch 两样本 t 检验（H0：dsaac::Vector 与 std::vector 均值相等）；\n";
    std::cout << "# diff95 为均值差 (our - std) 的 95% 置信区间；p<0.05 表示差异显著\n";
    for (const Row &r : rows) {
        dsaac::WelchResult w = dsaac::welch(r.our, r.stdVec);
        double mo = dsaac::mean(r.our), ms = dsaac::mean(r.stdVec);
        std::cout << "STAT " << r.scenario << " n=" << r.scale
                  << std::fixed << std::setprecision(3)
                  << " our_ms=" << mo << " std_ms=" << ms
                  << " ratio=" << (ms > 0 ? mo / ms : 0.0)
                  << " t=" << w.t << " df=" << w.df;
        std::cout << std::defaultfloat << std::setprecision(2) << " p=" << w.p;
        std::cout << std::fixed << std::setprecision(3)
                  << " diff95=[" << w.ciLow << "," << w.ciHigh << "]\n";
    }
    std::filesystem::create_directories("results");
    std::ofstream raw("results/vector_std.csv");
    if (!raw) throw std::runtime_error("无法写入原始计时数据");
    raw << "scenario,n,ops,implementation,repeat,time_ms\n" << std::setprecision(12);
    for (const auto& row : rows) {
        for (std::size_t i = 0; i < row.our.size(); ++i)
            raw << row.scenario << ',' << row.scale << ',' << row.ops << ",vector," << i + 1 << ',' << row.our[i] << '\n';
        for (std::size_t i = 0; i < row.stdVec.size(); ++i)
            raw << row.scenario << ',' << row.scale << ',' << row.ops << ",std_vector," << i + 1 << ',' << row.stdVec[i] << '\n';
    }
    raw.close();
    if (!raw) throw std::runtime_error("原始计时数据写入失败");
    return 0;
}