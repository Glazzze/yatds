#include "extended_queue.hpp"

#include <stdexcept>

/* START: extended_queue implementation */

template <typename T>
dsaac::ExtendedQueue<T>::ExtendedQueue(std::size_t capacity)
    : buffer_(capacity), front_(0), count_(0)
{
}

template <typename T>
bool dsaac::ExtendedQueue<T>::empty() const
{
    return count_ == 0;
}

template <typename T>
bool dsaac::ExtendedQueue<T>::full() const
{
    return count_ == buffer_.size();
}

template <typename T>
void dsaac::ExtendedQueue<T>::push(const T& value)
{
    if (full())
        throw std::overflow_error("队列已满");
    buffer_[(front_ + count_) % buffer_.size()] = value;
    count_++;
}

template <typename T>
void dsaac::ExtendedQueue<T>::pop()
{
    if (empty())
        throw std::out_of_range("队列为空");
    front_ = (front_ + 1) % buffer_.size();
    count_--;
}

template <typename T>
const T& dsaac::ExtendedQueue<T>::front() const
{
    if (empty())
        throw std::out_of_range("队列为空");
    return buffer_[front_];
}

/* ---------- 扩展功能 ---------- */

/* 队列长度（<queue> 对应 size） */
template <typename T>
std::size_t dsaac::ExtendedQueue<T>::size() const
{
    return count_;
}

/* 清空全部元素（<queue> 无直接 clear） */
template <typename T>
void dsaac::ExtendedQueue<T>::clear()
{
    front_ = 0;
    count_ = 0;
}

/* 交换两队列内容（对应 std::queue::swap） */
template <typename T>
void dsaac::ExtendedQueue<T>::swap(ExtendedQueue& other)
{
    buffer_.swap(other.buffer_);
    std::swap(front_, other.front_);
    std::swap(count_, other.count_);
}

/* 访问队尾元素（<queue> 对应 back） */
template <typename T>
const T& dsaac::ExtendedQueue<T>::back() const
{
    if (empty())
        throw std::out_of_range("队列为空");
    return buffer_[(front_ + count_ - 1) % buffer_.size()];
}

/* 查找 value 距队首的位置（队首为 1），未找到返回 -1 */
template <typename T>
int dsaac::ExtendedQueue<T>::search(const T& value) const
{
    for (std::size_t i = 0; i < count_; ++i) {
        if (buffer_[(front_ + i) % buffer_.size()] == value)
            return static_cast<int>(i + 1);
    }
    return -1;
}

/* 显式实例化 */
template class dsaac::ExtendedQueue<int>;
template class dsaac::ExtendedQueue<double>;
template class dsaac::ExtendedQueue<char>;

/* END */