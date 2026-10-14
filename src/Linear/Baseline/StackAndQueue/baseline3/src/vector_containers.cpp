#include "vector_containers.hpp"

#include <stdexcept>

/* START: vector_containers implementation */

/* ---------- VectorStack ---------- */

template <typename T>
bool dsaac::VectorStack<T>::empty() const
{
    return data_.empty();
}

template <typename T>
std::size_t dsaac::VectorStack<T>::size() const
{
    return data_.size();
}

template <typename T>
void dsaac::VectorStack<T>::push(const T& value)
{
    data_.push_back(value);
}

template <typename T>
void dsaac::VectorStack<T>::pop()
{
    if (data_.empty())
        throw std::out_of_range("栈为空");
    data_.pop_back();
}

template <typename T>
const T& dsaac::VectorStack<T>::top() const
{
    if (data_.empty())
        throw std::out_of_range("栈为空");
    return data_.back();
}

/* 转换为队列：栈顶→队首（按出栈顺序入队） */
template <typename T>
dsaac::VectorQueue<T> dsaac::VectorStack<T>::to_queue() const
{
    VectorQueue<T> q;
    for (auto it = data_.rbegin(); it != data_.rend(); ++it)
        q.push(*it);
    return q;
}

/* ---------- VectorQueue ---------- */

template <typename T>
bool dsaac::VectorQueue<T>::empty() const
{
    return head_ >= data_.size();
}

template <typename T>
std::size_t dsaac::VectorQueue<T>::size() const
{
    return data_.size() - head_;
}

template <typename T>
void dsaac::VectorQueue<T>::push(const T& value)
{
    data_.push_back(value);
}

template <typename T>
void dsaac::VectorQueue<T>::pop()
{
    if (empty())
        throw std::out_of_range("队列为空");
    head_++;
}

template <typename T>
const T& dsaac::VectorQueue<T>::front() const
{
    if (empty())
        throw std::out_of_range("队列为空");
    return data_[head_];
}

/* 转换为栈：队首→栈底（按出队顺序入栈） */
template <typename T>
dsaac::VectorStack<T> dsaac::VectorQueue<T>::to_stack() const
{
    VectorStack<T> s;
    for (std::size_t i = head_; i < data_.size(); ++i)
        s.push(data_[i]);
    return s;
}

/* 显式实例化 */
template class dsaac::VectorStack<int>;
template class dsaac::VectorQueue<int>;
template class dsaac::VectorStack<double>;
template class dsaac::VectorQueue<double>;
template class dsaac::VectorStack<char>;
template class dsaac::VectorQueue<char>;

/* END */