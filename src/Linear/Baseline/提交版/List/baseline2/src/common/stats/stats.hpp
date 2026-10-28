#ifndef DSAAC_STATS_HPP
#define DSAAC_STATS_HPP

/*
 * 轻量统计工具（header-only）：供性能基准程序计算均值/标准差、做 Welch 两样本 t 检验
 * 并给出均值差的 95% 置信区间。
 *
 * 采用 Welch t 检验而非 Student t 检验，因为自实现与 std::vector 的方差并不假定相等。
 * 需要 t 分布 CDF，用“正则化不完全 Beta 函数 + 连分式”实现（Numerical Recipes 的做法）。
 */

#include <cmath>
#include <cstddef>
#include <vector>
#include <limits>
#include <stdexcept>

namespace dsaac {

/* 样本均值 */
inline double mean(const std::vector<double> &v)
{
    double s = 0.0;
    for (double x : v)
        s += x;
    return v.empty() ? 0.0 : s / static_cast<double>(v.size());
}

/* 总体标准差（除以 n，与基准表格口径一致） */
inline double sdPop(const std::vector<double> &v)
{
    double m = mean(v), s = 0.0;
    for (double x : v)
        s += (x - m) * (x - m);
    return v.empty() ? 0.0 : std::sqrt(s / static_cast<double>(v.size()));
}

/* 样本方差（除以 n-1，用于 t 检验） */
inline double varSample(const std::vector<double> &v)
{
    if (v.size() < 2)
        return 0.0;
    double m = mean(v), s = 0.0;
    for (double x : v)
        s += (x - m) * (x - m);
    return s / static_cast<double>(v.size() - 1);
}

namespace detail {

/* 连分式求正则化不完全 Beta 函数的连分部分（Lentz 算法） */
inline double betacf(double a, double b, double x)
{
    const int MAXIT = 300;
    const double EPS = 3.0e-14, FPMIN = 1.0e-300;
    double qab = a + b, qap = a + 1.0, qam = a - 1.0;
    double c = 1.0;
    double d = 1.0 - qab * x / qap;
    if (std::fabs(d) < FPMIN) d = FPMIN;
    d = 1.0 / d;
    double h = d;
    for (int m = 1; m <= MAXIT; m++) {
        int m2 = 2 * m;
        double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
        d = 1.0 + aa * d;
        if (std::fabs(d) < FPMIN) d = FPMIN;
        c = 1.0 + aa / c;
        if (std::fabs(c) < FPMIN) c = FPMIN;
        d = 1.0 / d;
        h *= d * c;
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
        d = 1.0 + aa * d;
        if (std::fabs(d) < FPMIN) d = FPMIN;
        c = 1.0 + aa / c;
        if (std::fabs(c) < FPMIN) c = FPMIN;
        d = 1.0 / d;
        double del = d * c;
        h *= del;
        if (std::fabs(del - 1.0) < EPS) break;
    }
    return h;
}

/* 正则化不完全 Beta 函数 I_x(a, b) */
inline double betai(double a, double b, double x)
{
    if (x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;
    double bt = std::exp(std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b) +
                         a * std::log(x) + b * std::log(1.0 - x));
    if (x < (a + 1.0) / (a + b + 2.0))
        return bt * betacf(a, b, x) / a;
    return 1.0 - bt * betacf(b, a, 1.0 - x) / b;
}

/* t 分布累积分布函数 P(T <= t)，自由度 df */
inline double tCdf(double t, double df)
{
    double x = df / (df + t * t);
    double tail = 0.5 * betai(df / 2.0, 0.5, x);  // P(T > |t|)
    return t >= 0.0 ? 1.0 - tail : tail;
}

/* t 分布分位数（二分法求 tCdf(t,df)=p） */
inline double tQuantile(double p, double df)
{
    double lo = -1.0e3, hi = 1.0e3;
    for (int i = 0; i < 200; i++) {
        double mid = 0.5 * (lo + hi);
        if (tCdf(mid, df) < p)
            lo = mid;
        else
            hi = mid;
    }
    return 0.5 * (lo + hi);
}

} // namespace detail

/* Welch 两样本 t 检验结果 */
struct WelchResult {
    double t;        // t 统计量
    double df;       // Welch–Satterthwaite 自由度
    double p;        // 双侧 p 值（H0: 两总体均值相等）
    double diff;     // 均值差 mean(a) - mean(b)
    double ciLow;    // 均值差的置信区间下界
    double ciHigh;   // 均值差的置信区间上界
};

/* 对两组独立样本做 Welch t 检验，并给出均值差的 (1-alpha) 置信区间 */
inline WelchResult welch(const std::vector<double> &a, const std::vector<double> &b,
                         double conf = 0.95)
{
    if (a.size() < 2 || b.size() < 2 || !(conf > 0.0 && conf < 1.0))
        throw std::invalid_argument("Welch 检验要求至少两个样本及合法置信水平");
    double ma = mean(a), mb = mean(b);
    double va = varSample(a), vb = varSample(b);
    std::size_t na = a.size(), nb = b.size();
    double se2 = va / static_cast<double>(na) + vb / static_cast<double>(nb);
    if (se2 == 0.0) {
        // 两组都零方差时 t 检验不适用，不能误报显著/不显著。
        const double nan = std::numeric_limits<double>::quiet_NaN();
        return WelchResult{nan, nan, nan, ma - mb, ma - mb, ma - mb};
    }
    double t = (ma - mb) / std::sqrt(se2);
    double denom = 0.0;
    if (na > 1) denom += (va / na) * (va / na) / static_cast<double>(na - 1);
    if (nb > 1) denom += (vb / nb) * (vb / nb) / static_cast<double>(nb - 1);
    double df = denom > 0.0 ? se2 * se2 / denom : 1.0;
    // 直接计算双侧尾概率，避免 1-CDF 在极小 p 值时相消为 0。
    double p = detail::betai(df / 2.0, 0.5, df / (df + t * t));
    double tcrit = detail::tQuantile(1.0 - (1.0 - conf) / 2.0, df);
    double margin = tcrit * std::sqrt(se2);
    return WelchResult{t, df, p, ma - mb, ma - mb - margin, ma - mb + margin};
}

} // namespace dsaac

#endif // DSAAC_STATS_HPP