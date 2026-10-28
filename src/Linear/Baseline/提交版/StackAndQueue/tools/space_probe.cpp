/*
 * 空间探针：量化数组实现与链表实现"每元素内存开销"的差异。
 *
 * 做法：Linux 读取 /proc/self/statm，macOS 使用 task_info，取得当前常驻内存，在把 n 个 int 压入某结构**前后**
 * 各取一次 RSS，差值即该结构的实际驻留内存；再除以 n 得 bytes/elem。
 * 这比"链表每元素多一个指针"这类理论估算更接近真实开销（含分配器头部、
 * 对齐、分段容器的块指针等）。
 *
 * 编译（需与三个给定骨架的 .cpp 一起）：
 *   g++ -std=c++17 -O2 tools/space_probe.cpp \
 *       src/array_stack/stackar.cpp src/linked_stack/stackli.cpp \
 *       src/queue/queue.cpp baseline2/src/linked_queue.cpp -o /tmp/space_probe
 * 运行：
 *   /tmp/space_probe <array|linked|stdstack|circular|linkedq|stdqueue> <n>
 */
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <queue>
#include <stack>
#include <string>
#include <unistd.h>
#include <stdexcept>
#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include "../baseline2/src/linked_queue.hpp"
#include "../src/array_stack/stackar.hpp"
#include "../src/linked_stack/stackli.hpp"
#include "../src/queue/queue.hpp"

// 采样当前 RSS，非进程历史峰值；支持 Linux / macOS，读取失败不能假装为 0。
static long rss_kb()
{
#ifdef __APPLE__
    mach_task_basic_info_data_t info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS)
        throw std::runtime_error("无法读取 macOS 当前 RSS");
    return static_cast<long>(info.resident_size / 1024);
#else
    std::ifstream f("/proc/self/statm");
    long size = 0, resident = 0;
    const long page_size = sysconf(_SC_PAGESIZE);
    if (!(f >> size >> resident) || page_size <= 0)
        throw std::runtime_error("无法读取当前 RSS: /proc/self/statm");
    return resident * page_size / 1024;
#endif
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "用法: space_probe <mode> <n>\n");
        return 2;
    }
    const std::string mode = argv[1];
    const long n = std::atol(argv[2]);
    if (n <= 0) {
        std::fprintf(stderr, "n 必须为正数\n");
        return 2;
    }
    const long before = rss_kb();
    long delta = 0;

    if (mode == "array") {
        dsaac::ArrayStack<int> s(static_cast<std::size_t>(n));
        for (long i = 0; i < n; ++i) s.push(static_cast<int>(i));
        delta = rss_kb() - before;
    } else if (mode == "linked") {
        dsaac::LinkedStack<int> s;
        for (long i = 0; i < n; ++i) s.push(static_cast<int>(i));
        delta = rss_kb() - before;
    } else if (mode == "stdstack") {
        std::stack<int> s;
        for (long i = 0; i < n; ++i) s.push(static_cast<int>(i));
        delta = rss_kb() - before;
    } else if (mode == "circular") {
        dsaac::CircularQueue<int> q(static_cast<std::size_t>(n));
        for (long i = 0; i < n; ++i) q.push(static_cast<int>(i));
        delta = rss_kb() - before;
    } else if (mode == "linkedq") {
        dsaac::LinkedQueue<int> q;
        for (long i = 0; i < n; ++i) q.push(static_cast<int>(i));
        delta = rss_kb() - before;
    } else if (mode == "stdqueue") {
        std::queue<int> q;
        for (long i = 0; i < n; ++i) q.push(static_cast<int>(i));
        delta = rss_kb() - before;
    } else {
        std::fprintf(stderr, "未知 mode: %s\n", mode.c_str());
        return 2;
    }

    std::printf("RSS %-9s n=%ld  delta_kb=%ld  bytes_per_elem=%.2f\n",
                mode.c_str(), n, delta, n ? static_cast<double>(delta) * 1024.0 / n : 0.0);
    return 0;
}