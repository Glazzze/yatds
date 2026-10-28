#include "../common/stats/stats.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

int main()
{
    int checks = 0;
    auto expect = [&](bool condition) {
        if (!condition) throw std::runtime_error("统计工具回归失败");
        ++checks;
    };
    // 自由度 1 的 t 分布为标准 Cauchy，F(1)=0.75 是解析值。
    expect(std::fabs(dsaac::detail::tCdf(1.0, 1.0) - 0.75) < 1e-12);
    expect(std::fabs(dsaac::detail::tQuantile(0.75, 1.0) - 1.0) < 1e-10);
    auto same = dsaac::welch({1, 2, 3}, {1, 2, 3});
    expect(std::fabs(same.p - 1.0) < 1e-12);
    auto large = dsaac::welch({10000, 10001, 10002}, {1, 2, 3});
    expect(large.p > 0.0 && large.p < 1e-12);
    auto reversed = dsaac::welch({1, 2, 3}, {10000, 10001, 10002});
    expect(std::fabs(reversed.p - large.p) < 1e-20 && reversed.t == -large.t);
    expect(std::isnan(dsaac::welch({1, 1}, {2, 2}).p));
    bool rejected = false;
    try { dsaac::welch({1}, {2, 3}); } catch (const std::invalid_argument&) { rejected = true; }
    expect(rejected);
    std::cout << "Statistics regression: " << checks << " PASS\n";
}
