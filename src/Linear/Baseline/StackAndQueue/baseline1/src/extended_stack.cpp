#include "extended_stack.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>

/* START: extended_stack implementation */

/* =========================== 扩展数组栈 =========================== */

template <typename T>
dsaac::ExtendedArrayStack<T>::ExtendedArrayStack(std::size_t capacity)
    : capacity_(capacity)
{
    values_.reserve(capacity);
}

template <typename T>
bool dsaac::ExtendedArrayStack<T>::empty() const
{
    return values_.empty();
}

template <typename T>
bool dsaac::ExtendedArrayStack<T>::full() const
{
    return values_.size() >= capacity_;
}

template <typename T>
std::size_t dsaac::ExtendedArrayStack<T>::size() const
{
    return values_.size();
}

template <typename T>
void dsaac::ExtendedArrayStack<T>::push(const T& value)
{
    if (full())
        throw std::overflow_error("栈已满");
    values_.push_back(value);
}

template <typename T>
void dsaac::ExtendedArrayStack<T>::pop()
{
    if (empty())
        throw std::out_of_range("栈为空");
    values_.pop_back();
}

template <typename T>
const T& dsaac::ExtendedArrayStack<T>::top() const
{
    if (empty())
        throw std::out_of_range("栈为空");
    return values_.back();
}

/* 清空全部元素（不缩容，保留预留容量） */
template <typename T>
void dsaac::ExtendedArrayStack<T>::clear()
{
    values_.clear();
}

/* 交换两栈内容（O(1)：交换底层 vector 与容量） */
template <typename T>
void dsaac::ExtendedArrayStack<T>::swap(ExtendedArrayStack& other)
{
    values_.swap(other.values_);
    std::swap(capacity_, other.capacity_);
}

/* 查找 value 距栈顶的位置（栈顶为 1），未找到返回 -1（O(n)） */
template <typename T>
int dsaac::ExtendedArrayStack<T>::search(const T& value) const
{
    std::size_t n = values_.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (values_[n - 1 - i] == value)  // 从尾部（栈顶）向前扫描
            return static_cast<int>(i + 1);
    }
    return -1;
}

/* 访问距栈顶第 depth 个元素（栈顶为 1），越界抛 out_of_range（O(1)） */
template <typename T>
T dsaac::ExtendedArrayStack<T>::peek(std::size_t depth) const
{
    if (depth < 1 || depth > values_.size())
        throw std::out_of_range("访问位置越界");
    return values_[values_.size() - depth];
}

/* 原地反转整栈（O(n)） */
template <typename T>
void dsaac::ExtendedArrayStack<T>::reverse()
{
    std::reverse(values_.begin(), values_.end());
}

/* =========================== 扩展链式栈 =========================== */

template <typename T>
bool dsaac::ExtendedLinkedStack<T>::empty() const
{
    return values_.empty();
}

template <typename T>
std::size_t dsaac::ExtendedLinkedStack<T>::size() const
{
    return static_cast<std::size_t>(std::distance(values_.begin(), values_.end()));
}

template <typename T>
void dsaac::ExtendedLinkedStack<T>::push(const T& value)
{
    values_.push_front(value);
}

template <typename T>
void dsaac::ExtendedLinkedStack<T>::pop()
{
    if (empty())
        throw std::out_of_range("栈为空");
    values_.pop_front();
}

template <typename T>
const T& dsaac::ExtendedLinkedStack<T>::top() const
{
    if (empty())
        throw std::out_of_range("栈为空");
    return values_.front();
}

template <typename T>
void dsaac::ExtendedLinkedStack<T>::clear()
{
    values_.clear();
}

template <typename T>
void dsaac::ExtendedLinkedStack<T>::swap(ExtendedLinkedStack& other)
{
    values_.swap(other.values_);
}

/* 查找 value 距栈顶的位置（栈顶为 1），未找到返回 -1（O(n)） */
template <typename T>
int dsaac::ExtendedLinkedStack<T>::search(const T& value) const
{
    int pos = 1;
    for (const T& v : values_) {  // 从头部（栈顶）向后扫描
        if (v == value)
            return pos;
        pos++;
    }
    return -1;
}

/* 访问距栈顶第 depth 个元素，越界抛 out_of_range（O(depth)） */
template <typename T>
T dsaac::ExtendedLinkedStack<T>::peek(std::size_t depth) const
{
    if (depth < 1 || depth > size())
        throw std::out_of_range("访问位置越界");
    auto it = values_.begin();
    std::advance(it, static_cast<long>(depth - 1));
    return *it;
}

/* 原地反转整栈（O(n)：forward_list 自带 reverse） */
template <typename T>
void dsaac::ExtendedLinkedStack<T>::reverse()
{
    values_.reverse();
}

/* 显式实例化，保证与测试程序分开编译时链接通过 */
template class dsaac::ExtendedArrayStack<int>;
template class dsaac::ExtendedArrayStack<double>;
template class dsaac::ExtendedArrayStack<char>;
template class dsaac::ExtendedLinkedStack<int>;
template class dsaac::ExtendedLinkedStack<double>;
template class dsaac::ExtendedLinkedStack<char>;

/* END */