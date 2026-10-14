#ifndef DSAAC_EXTENDED_QUEUE_HPP
#define DSAAC_EXTENDED_QUEUE_HPP

#include <cstddef>
#include <vector>

namespace dsaac {

/*
 * 扩展队列：在给定 queue.hpp 的基础接口上，参考 <queue> 补充五个功能——
 * size / clear / swap / back / search。因给定 queue.hpp 不可修改，
 * 扩展功能以独立模板类实现（自带循环缓冲存储，front + size 索引法）。
 */
template <typename T>
class ExtendedQueue {
public:
    explicit ExtendedQueue(std::size_t capacity);

    bool empty() const;
    bool full() const;
    void push(const T& value);
    void pop();
    const T& front() const;

    /* ---------- 扩展功能 ---------- */
    std::size_t size() const;
    void clear();
    void swap(ExtendedQueue& other);
    const T& back() const;
    int search(const T& value) const;

private:
    std::vector<T> buffer_;
    std::size_t front_ = 0;
    std::size_t count_ = 0;
};

} // namespace dsaac

template <typename T>
using ExtendedQueue = dsaac::ExtendedQueue<T>;

#endif