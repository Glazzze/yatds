/*
 * Baseline 任务 2 · 小任务 4：可变数组 vs 链表的性能对比程序
 *
 * 二者都是可以动态扩容的线性结构，但在不同操作上的代价差异很大。
 * 本程序在 4 种典型场景下对比 dsaac::Vector 与 LinkedList 的耗时：
 *   1. append   ：尾部追加 n 次（原型：都是 O(1)）
 *   2. prepend  ：头部插入 m 次（可变数组每次 O(n) 搬移，链表 O(1)）
 *   3. traverse ：建表后顺序遍历 n 次（都可 O(n)，考察缓存局部性）
 *   4. random   ：建表后随机访问 k 次（可变数组 O(1)，链表 O(n)）
 *
 * 每项重复 REPEAT 次，报告均值与总体标准差，并做 Welch 两样本 t 检验（STAT 行），
 * 输出 PERF 统计行。
 */

/* 复用 小任务 1 的可变数组实现与链表实现（相对路径） */
#include "../task1_vector/vector.hpp"
#include "../common/linked_list/linked_list.hpp"
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


/* ---------- 可变数组侧 ---------- */

static double vecAppend(int n)
{
    dsaac::Vector<int> v;
    return dsaac::benchmark::measure([&] {
        for (int i = 0; i < n; ++i) {
            v.push_back(i);
            dsaac::benchmark::escape(&v);
        }
    });
}

static double vecPrepend(int m)
{
    dsaac::Vector<int> v;
    return dsaac::benchmark::measure([&] {
        for (int i = 0; i < m; ++i) {
            v.insert(0, i);
            dsaac::benchmark::escape(&v);
        }
    });
}

static double vecTraverse(int n)
{
    dsaac::Vector<int> v;
    for (int i = 0; i < n; ++i) v.push_back(i);
    long long sum = 0;
    double elapsed = dsaac::benchmark::measure([&] {
        for (int i = 0; i < n; ++i) sum += v[static_cast<std::size_t>(i)];
        dsaac::benchmark::consume(sum);
    });
    if (sum != 1LL * n * (n - 1) / 2) throw std::runtime_error("遍历校验和错误");
    return elapsed;
}

static double vecRandom(int n, int k)
{
    dsaac::Vector<int> v;
    for (int i = 0; i < n; ++i) v.push_back(i);
    std::vector<std::size_t> indices;
    unsigned int seed = 20260314u;
    long long expected = 0;
    for (int i = 0; i < k; ++i) {
        seed = seed * 1103515245u + 12345u;
        indices.push_back(seed % static_cast<unsigned int>(n));
        expected += static_cast<long long>(indices.back());
    }
    long long sum = 0;
    double elapsed = dsaac::benchmark::measure([&] {
        for (auto index : indices) sum += v[index];
        dsaac::benchmark::consume(sum);
    });
    if (sum != expected) throw std::runtime_error("随机访问校验和错误");
    return elapsed;
}

/* ---------- 链表侧 ---------- */

static double listAppend(int n)
{
    LinkedList<int> v;
    return dsaac::benchmark::measure([&] {
        for (int i = 0; i < n; ++i) {
            v.push_back(i);
            dsaac::benchmark::escape(&v);
        }
    });
}

static double listPrepend(int m)
{
    LinkedList<int> v;
    return dsaac::benchmark::measure([&] {
        for (int i = 0; i < m; ++i) {
            v.push_front(i);
            dsaac::benchmark::escape(&v);
        }
    });
}

static double listTraverse(int n)
{
    LinkedList<int> v;
    for (int i = 0; i < n; ++i) v.push_back(i);
    long long sum = 0;
    double elapsed = dsaac::benchmark::measure([&] {
        for (auto p = v.head_node(); p != nullptr; p = p->next) sum += p->val;
        dsaac::benchmark::consume(sum);
    });
    if (sum != 1LL * n * (n - 1) / 2) throw std::runtime_error("遍历校验和错误");
    return elapsed;
}

static double listRandom(int n, int k)
{
    LinkedList<int> v;
    for (int i = 0; i < n; ++i) v.push_back(i);
    std::vector<std::size_t> indices;
    unsigned int seed = 20260314u;
    long long expected = 0;
    for (int i = 0; i < k; ++i) {
        seed = seed * 1103515245u + 12345u;
        indices.push_back(seed % static_cast<unsigned int>(n));
        expected += static_cast<long long>(indices.back());
    }
    long long sum = 0;
    double elapsed = dsaac::benchmark::measure([&] {
        for (auto index : indices) sum += v.at(index);
        dsaac::benchmark::consume(sum);
    });
    if (sum != expected) throw std::runtime_error("随机访问校验和错误");
    return elapsed;
}

/* 预热一次，再交替测量顺序；CSV 中记录正式重复的原始样本。 */
template <typename VecFn, typename ListFn>
static std::pair<std::vector<double>, std::vector<double>> samplesPair(VecFn vec, ListFn list)
{
    vec(); list();
    std::pair<std::vector<double>, std::vector<double>> samples;
    for (int i = 0; i < REPEAT; ++i) {
        if (i % 2 == 0) {
            samples.first.push_back(vec()); samples.second.push_back(list());
        } else {
            samples.second.push_back(list()); samples.first.push_back(vec());
        }
    }
    return samples;
}

/* 由原始样本得到均值与总体标准差 */
static Stat summarize(const std::vector<double> &samples)
{
    return Stat{dsaac::mean(samples), dsaac::sdPop(samples)};
}

int main()
{
    std::ios::sync_with_stdio(false);

    /* 保存每次测量结果（原始样本），供汇总比值与显著性检验复用 */
    struct Row {
        std::string scenario;
        int scale;
        int ops;
        std::vector<double> vec;
        std::vector<double> list;
    };
    std::vector<Row> rows;

    for (int n : {100000, 1000000}) {
        /* 头部插入为 O(n^2)（可变数组），规模缩小；随机访问链表为 O(k*n)，访问次数取小 */
        int prependM = n / 100;
        int randomK = 2000;
        std::cout << "# 数据规模 n=" << n << "（头部插入 m=" << prependM
                  << "，随机访问 k=" << randomK << "），每项重复 " << REPEAT
                  << " 次，报告均值与标准差\n";

        auto aSamples = samplesPair([n] { return vecAppend(n); }, [n] { return listAppend(n); });
        const auto& aVec = aSamples.first;
        const auto& aList = aSamples.second;
        Stat aV = summarize(aVec), aL = summarize(aList);
        rows.push_back({"append", n, n, aVec, aList});
        std::cout << "PERF vector_like_append n=" << n << " ops=" << n
                  << " time_ms=" << std::fixed << std::setprecision(3) << aV.mean
                  << " sd=" << aV.sd << '\n';
        std::cout << "PERF linked_list_append n=" << n << " ops=" << n
                  << " time_ms=" << aL.mean << " sd=" << aL.sd << '\n';

        auto pSamples = samplesPair([prependM] { return vecPrepend(prependM); }, [prependM] { return listPrepend(prependM); });
        const auto& pVec = pSamples.first;
        const auto& pList = pSamples.second;
        Stat pV = summarize(pVec), pL = summarize(pList);
        rows.push_back({"prepend", prependM, prependM, pVec, pList});
        std::cout << "PERF vector_like_prepend n=" << prependM << " ops=" << prependM
                  << " time_ms=" << pV.mean << " sd=" << pV.sd << '\n';
        std::cout << "PERF linked_list_prepend n=" << prependM << " ops=" << prependM
                  << " time_ms=" << pL.mean << " sd=" << pL.sd << '\n';

        auto tSamples = samplesPair([n] { return vecTraverse(n); }, [n] { return listTraverse(n); });
        const auto& tVec = tSamples.first;
        const auto& tList = tSamples.second;
        Stat tV = summarize(tVec), tL = summarize(tList);
        rows.push_back({"traverse", n, n, tVec, tList});
        std::cout << "PERF vector_like_traverse n=" << n << " ops=" << n
                  << " time_ms=" << tV.mean << " sd=" << tV.sd << '\n';
        std::cout << "PERF linked_list_traverse n=" << n << " ops=" << n
                  << " time_ms=" << tL.mean << " sd=" << tL.sd << '\n';

        auto rSamples = samplesPair([n, randomK] { return vecRandom(n, randomK); }, [n, randomK] { return listRandom(n, randomK); });
        const auto& rVec = rSamples.first;
        const auto& rList = rSamples.second;
        Stat rV = summarize(rVec), rL = summarize(rList);
        rows.push_back({"random", n, randomK, rVec, rList});
        std::cout << "PERF vector_like_random n=" << n << " ops=" << randomK
                  << " time_ms=" << rV.mean << " sd=" << rV.sd << '\n';
        std::cout << "PERF linked_list_random n=" << n << " ops=" << randomK
                  << " time_ms=" << rL.mean << " sd=" << rL.sd << '\n';
        std::cout << '\n';
    }

    std::cout << "# 汇总：time_ms 比值 = 链表耗时 / 可变数组耗时（>1 表示链表更慢）\n";
    for (const Row &r : rows) {
        double mv = dsaac::mean(r.vec), ml = dsaac::mean(r.list);
        std::cout << "PERF ratio_" << r.scenario << " n=" << r.scale
                  << " time_ms=" << std::fixed << std::setprecision(3)
                  << (mv > 0 ? ml / mv : 0.0) << '\n';
    }

    /* 统计显著性：Welch 两样本 t 检验（H0：链表与可变数组均值相等）+ 均值差的 95% 置信区间 */
    std::cout << "# 统计显著性：Welch 两样本 t 检验（H0：链表与可变数组均值相等）；\n";
    std::cout << "# diff95 为均值差 (list - vec) 的 95% 置信区间；p<0.05 表示差异显著\n";
    for (const Row &r : rows) {
        dsaac::WelchResult w = dsaac::welch(r.list, r.vec);
        double mv = dsaac::mean(r.vec), ml = dsaac::mean(r.list);
        std::cout << "STAT " << r.scenario << " n=" << r.scale
                  << std::fixed << std::setprecision(3)
                  << " vec_ms=" << mv << " list_ms=" << ml
                  << " ratio=" << (mv > 0 ? ml / mv : 0.0)
                  << " t=" << w.t << " df=" << w.df;
        std::cout << std::defaultfloat << std::setprecision(2) << " p=" << w.p;
        std::cout << std::fixed << std::setprecision(3)
                  << " diff95=[" << w.ciLow << "," << w.ciHigh << "]\n";
    }
    std::filesystem::create_directories("results");
    std::ofstream raw("results/array_list.csv");
    if (!raw) throw std::runtime_error("无法写入原始计时数据");
    raw << "scenario,n,ops,implementation,repeat,time_ms\n" << std::setprecision(12);
    for (const auto& row : rows) {
        for (std::size_t i = 0; i < row.vec.size(); ++i)
            raw << row.scenario << ',' << row.scale << ',' << row.ops << ",vector," << i + 1 << ',' << row.vec[i] << '\n';
        for (std::size_t i = 0; i < row.list.size(); ++i)
            raw << row.scenario << ',' << row.scale << ',' << row.ops << ",linked_list," << i + 1 << ',' << row.list[i] << '\n';
    }
    raw.close();
    if (!raw) throw std::runtime_error("原始计时数据写入失败");
    return 0;
}