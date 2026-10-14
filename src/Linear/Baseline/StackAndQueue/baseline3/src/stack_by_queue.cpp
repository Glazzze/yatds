#include "stack_by_queue.hpp"

#include <stdexcept>

/* START: stack_by_queue implementation */

template <typename T>
bool dsaac::StackByQueue<T>::empty() const
{
    return q_.empty();
}

template <typename T>
std::size_t dsaac::StackByQueue<T>::size() const
{
    return q_.size();
}

template <typename T>
void dsaac::StackByQueue<T>::push(const T& value)
{
    q_.push(value);
}

/* 出栈：旋转 n-1 次使栈顶（原队尾）到队首，再出队 */
template <typename T>
void dsaac::StackByQueue<T>::pop()
{
    if (q_.empty())
        throw std::out_of_range("栈为空");
    for (std::size_t i = 0; i + 1 < q_.size(); ++i) {
        q_.push(q_.front());
        q_.pop();
    }
    q_.pop();
}

/* 栈顶：最后入队者即队尾，O(1) */
template <typename T>
const T& dsaac::StackByQueue<T>::top() const
{
    if (q_.empty())
        throw std::out_of_range("栈为空");
    return q_.back();
}

template class dsaac::StackByQueue<int>;
template class dsaac::StackByQueue<double>;
template class dsaac::StackByQueue<char>;

/* END */