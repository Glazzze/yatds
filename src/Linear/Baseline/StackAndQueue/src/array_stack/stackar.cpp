#include "stackar.hpp"

#include <stdexcept>

/* START: array_stack implementation */

/*
 * 数组栈实现：以 vector 尾部为栈顶，push 即 push_back、pop 即 pop_back。
 *
 * 与动态扩容版本不同，本实现维护"固定容量"语义：构造时一次性
 * reserve 出全部空间，此后 push 前先检查 full()，保证元素个数
 * 永远不会超过构造容量，vector 也就永远不会自动扩容。
 */

/* 构造函数：预留容量，作为栈的最大元素个数上限 */
template <typename T>
dsaac::ArrayStack<T>::ArrayStack(std::size_t capacity)
{
    // reserve 只申请内存不构造元素；之后 push 均以 push_back 进行，
    // 只要保证 size <= capacity，capacity 就不会被动增长
    values_.reserve(capacity);
}

/* 判断栈是否为空 */
template <typename T>
bool dsaac::ArrayStack<T>::empty() const
{
    return values_.empty();
}

/*
 * 判断栈是否已满：当前元素个数达到构造时设定的容量即为满。
 *
 * 成员变量只有 values_ 一个（见 stackar.hpp），容量没有单独存放，
 * 好在 full() 成立时 push 会被拒绝、vector 绝不扩容，
 * 因此 values_.capacity() 恒等于构造时的 reserve 值，可以放心用作容量。
 */
template <typename T>
bool dsaac::ArrayStack<T>::full() const
{
    return values_.size() >= values_.capacity();
}

/* 入栈：满栈时抛出上溢异常，否则把元素压入栈顶 */
template <typename T>
void dsaac::ArrayStack<T>::push(const T& value)
{
    if (full())
        throw std::overflow_error("栈已满");
    values_.push_back(value);
}

/* 出栈：空栈时抛出下溢异常，否则弹出栈顶（不返回值，取值请先 top()） */
template <typename T>
void dsaac::ArrayStack<T>::pop()
{
    if (empty())
        throw std::out_of_range("栈为空");
    values_.pop_back();
}

/* 访问栈顶元素：返回只读引用，不弹出、不修改栈 */
template <typename T>
const T& dsaac::ArrayStack<T>::top() const
{
    if (empty())
        throw std::out_of_range("栈为空");
    return values_.back();
}

/* 显式实例化常用类型，保证 stackar.cpp 与测试程序分开编译时链接通过 */
template class dsaac::ArrayStack<int>;
template class dsaac::ArrayStack<double>;
template class dsaac::ArrayStack<char>;

/* END */
