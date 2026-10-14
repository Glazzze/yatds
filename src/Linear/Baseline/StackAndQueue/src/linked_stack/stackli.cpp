#include "stackli.hpp"

#include <stdexcept>

/* START: linked_stack implementation */

/*
 * 链式栈实现：以 forward_list 的头部为栈顶。
 * push 用头插（push_front，O(1)），pop 摘除头部（pop_front，O(1)）。
 *
 * 与数组栈不同，链表按需分配节点，没有固定容量上限，因此接口中
 * 不提供 full()——只要内存允许，push 永远成功。
 */

/* 判断栈是否为空 */
template <typename T>
bool dsaac::LinkedStack<T>::empty() const
{
    return values_.empty();
}

/* 入栈：头插法，新元素成为新的栈顶 */
template <typename T>
void dsaac::LinkedStack<T>::push(const T& value)
{
    values_.push_front(value);
}

/* 出栈：空栈抛下溢异常，否则弹出栈顶（不返回值，取值请先 top()） */
template <typename T>
void dsaac::LinkedStack<T>::pop()
{
    if (values_.empty())
        throw std::out_of_range("栈为空");
    values_.pop_front();
}

/* 访问栈顶元素：返回只读引用，不弹出、不修改栈 */
template <typename T>
const T& dsaac::LinkedStack<T>::top() const
{
    if (values_.empty())
        throw std::out_of_range("栈为空");
    return values_.front();
}

/* 显式实例化常用类型，保证 stackli.cpp 与测试程序分开编译时链接通过 */
template class dsaac::LinkedStack<int>;
template class dsaac::LinkedStack<double>;
template class dsaac::LinkedStack<char>;

/* END */