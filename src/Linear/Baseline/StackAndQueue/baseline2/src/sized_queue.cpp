#include "sized_queue.hpp"

#include <stdexcept>

/* START: sized_queue implementation */

/* 构造：缓冲区恰好 capacity 个槽（不牺牲空槽） */
template <typename T>
dsaac::SizedQueue<T>::SizedQueue(std::size_t capacity)
    : buffer_(capacity), front_(0), count_(0)
{
}

/* 空：【size 计数法】的判空——元素个数为 0 即空 */
template <typename T>
bool dsaac::SizedQueue<T>::empty() const
{
    return count_ == 0;
}

/* 满：元素个数达到缓冲区容量 */
template <typename T>
bool dsaac::SizedQueue<T>::full() const
{
    return count_ == buffer_.size();
}

template <typename T>
std::size_t dsaac::SizedQueue<T>::size() const
{
    return count_;
}

/* 入队：写入 rear = (front + count) % capacity，再增加计数 */
template <typename T>
void dsaac::SizedQueue<T>::push(const T& value)
{
    if (full())
        throw std::overflow_error("队列已满");
    std::size_t rear = (front_ + count_) % buffer_.size();
    buffer_[rear] = value;
    count_++;
}

/* 出队：front 前进一格（取模回绕），减少计数 */
template <typename T>
void dsaac::SizedQueue<T>::pop()
{
    if (empty())
        throw std::out_of_range("队列为空");
    front_ = (front_ + 1) % buffer_.size();
    count_--;
}

/* 访问队首：只读引用，不出队 */
template <typename T>
const T& dsaac::SizedQueue<T>::front() const
{
    if (empty())
        throw std::out_of_range("队列为空");
    return buffer_[front_];
}

/* 显式实例化 */
template class dsaac::SizedQueue<int>;
template class dsaac::SizedQueue<double>;
template class dsaac::SizedQueue<char>;

/* END */