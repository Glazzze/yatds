#ifndef DSAAC_BENCHMARK_HPP
#define DSAAC_BENCHMARK_HPP
#include <atomic>
#include <chrono>
#include <cstddef>

namespace dsaac {
namespace benchmark {
// GCC/Clang 编译器屏障，使容器及其可达存储在测量点保持可观察。
inline void escape(const void* address)
{
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "g"(address) : "memory");
#else
    static const void* volatile sink;
    sink = address;
    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}
inline void consume(long long value)
{
    static volatile long long checksum;
    checksum = value;
    (void)checksum;
}
template <typename Fn>
double measure(Fn&& operation)
{
    const auto start = std::chrono::steady_clock::now();
    operation();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}
} // namespace benchmark
} // namespace dsaac
#endif
