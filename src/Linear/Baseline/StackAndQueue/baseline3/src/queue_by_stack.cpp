#include "queue_by_stack.hpp"

#include <stdexcept>

/* START: queue_by_stack implementation */

/* 把 in_ 的元素全部倒入 out_（队首来到 out_ 栈顶） */
template <typename T>
void dsaac::QueueByStack<T>::transfer() const
{
    if (out_.empty()) {
        while (!in_.empty()) {
            out_.push(in_.top());
            in_.pop();
        }
    }
}

template <typename T>
bool dsaac::QueueByStack<T>::empty() const
{
    return in_.empty() && out_.empty();
}

template <typename T>
std::size_t dsaac::QueueByStack<T>::size() const
{
    return in_.size() + out_.size();
}

template <typename T>
void dsaac::QueueByStack<T>::push(const T& value)
{
    in_.push(value);
}

template <typename T>
void dsaac::QueueByStack<T>::pop()
{
    transfer();
    if (out_.empty())
        throw std::out_of_range("队列为空");
    out_.pop();
}

template <typename T>
const T& dsaac::QueueByStack<T>::front() const
{
    transfer();
    if (out_.empty())
        throw std::out_of_range("队列为空");
    return out_.top();
}

template class dsaac::QueueByStack<int>;
template class dsaac::QueueByStack<double>;
template class dsaac::QueueByStack<char>;

/* END */