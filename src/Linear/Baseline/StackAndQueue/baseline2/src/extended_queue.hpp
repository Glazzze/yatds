#ifndef DSAAC_EXTENDED_QUEUE_HPP
#define DSAAC_EXTENDED_QUEUE_HPP

#include <cstddef>
#include <vector>

namespace dsaac {

/*
 * 扩展队列：在给定 queue.hpp 的基础接口上，参考 <queue> 补充五个功能——
 * size / clear / swap / back / search。因给定 queue.hpp 不可修改，
 * 扩展功能以独立模板类实现（自带循环缓冲存储，front + size 索引法）。
 *
 * 为对齐 <queue> 的接口语义，另提供**非成员** swap 与六个比较运算符
 * （== != < <= > >=，按"队首→队尾"字典序，与 std::queue 一致）；
 * to_vector()（队首→队尾）供比较运算符使用。
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
    std::vector<T> to_vector() const;  // 队首→队尾

private:
    std::vector<T> buffer_;
    std::size_t front_ = 0;
    std::size_t count_ = 0;
};

/* ---------- 非成员 swap ---------- */
template <typename T>
void swap(ExtendedQueue<T>& a, ExtendedQueue<T>& b) { a.swap(b); }

/* ---------- 六个比较运算符（按"队首→队尾"字典序，与 std::queue 一致）---------- */
template <typename T>
bool operator==(const ExtendedQueue<T>& a, const ExtendedQueue<T>& b)
{ return a.to_vector() == b.to_vector(); }
template <typename T>
bool operator!=(const ExtendedQueue<T>& a, const ExtendedQueue<T>& b)
{ return !(a == b); }
template <typename T>
bool operator<(const ExtendedQueue<T>& a, const ExtendedQueue<T>& b)
{ return a.to_vector() < b.to_vector(); }
template <typename T>
bool operator<=(const ExtendedQueue<T>& a, const ExtendedQueue<T>& b)
{ return !(b < a); }
template <typename T>
bool operator>(const ExtendedQueue<T>& a, const ExtendedQueue<T>& b)
{ return b < a; }
template <typename T>
bool operator>=(const ExtendedQueue<T>& a, const ExtendedQueue<T>& b)
{ return !(a < b); }

} // namespace dsaac

template <typename T>
using ExtendedQueue = dsaac::ExtendedQueue<T>;

#endif