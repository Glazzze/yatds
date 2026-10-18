#ifndef DSAAC_EXTENDED_STACK_HPP
#define DSAAC_EXTENDED_STACK_HPP

#include <cstddef>
#include <forward_list>
#include <vector>

namespace dsaac {

/*
 * 扩展栈：在给定 stackar.hpp / stackli.hpp 的基础接口上，参考 <stack>
 * 补充五个功能——clear / swap / search / peek / reverse。
 *
 * 因给定头文件不可修改，扩展功能无法加进 ArrayStack / LinkedStack 本身，
 * 故这里以独立模板类实现（自带存储），接口风格与给定骨架保持一致。
 * 新增成员函数：
 *   clear()              删除全部元素
 *   swap(other)          交换两栈内容
 *   search(value)        元素距栈顶的 1 基位置，未找到返回 -1
 *   peek(depth)          访问距栈顶第 depth 个元素（栈顶为 1），越界抛异常
 *   reverse()            原地反转整栈
 *   to_vector()          以"栈底→栈顶"顺序导出（供比较运算符使用），O(n)
 *
 * 为对齐 <stack> 的接口语义，另提供**非成员** swap 与六个比较运算符
 * （== != < <= > >=，按"栈底→栈顶"字典序，与 std::stack 一致）。
 *
 * 统一的复杂度契约（两实现一致，均为上界保证）：
 *   search(v)  —— O(n)
 *   peek(depth)—— O(depth)（数组实现实际为 O(1)，为链表实现的上界）
 */

template <typename T>
class ExtendedArrayStack {
public:
    explicit ExtendedArrayStack(std::size_t capacity);

    bool empty() const;
    bool full() const;
    std::size_t size() const;
    void push(const T& value);
    void pop();
    const T& top() const;

    /* ---------- 扩展功能 ---------- */
    void clear();
    void swap(ExtendedArrayStack& other);
    int search(const T& value) const;
    T peek(std::size_t depth) const;
    void reverse();
    std::vector<T> to_vector() const;  // 栈底→栈顶

private:
    std::vector<T> values_;
    std::size_t capacity_;
};

template <typename T>
class ExtendedLinkedStack {
public:
    bool empty() const;
    std::size_t size() const;
    void push(const T& value);
    void pop();
    const T& top() const;

    /* ---------- 扩展功能 ---------- */
    void clear();
    void swap(ExtendedLinkedStack& other);
    int search(const T& value) const;
    T peek(std::size_t depth) const;
    void reverse();
    std::vector<T> to_vector() const;  // 栈底→栈顶

private:
    std::forward_list<T> values_;
};

/* ---------- 非成员 swap（对齐 std::swap 的两参数调用形式）---------- */

template <typename T>
void swap(ExtendedArrayStack<T>& a, ExtendedArrayStack<T>& b) { a.swap(b); }

template <typename T>
void swap(ExtendedLinkedStack<T>& a, ExtendedLinkedStack<T>& b) { a.swap(b); }

/* ---------- 六个比较运算符（按"栈底→栈顶"字典序，与 std::stack 一致）---------- */

template <typename T>
bool operator==(const ExtendedArrayStack<T>& a, const ExtendedArrayStack<T>& b)
{ return a.to_vector() == b.to_vector(); }
template <typename T>
bool operator!=(const ExtendedArrayStack<T>& a, const ExtendedArrayStack<T>& b)
{ return !(a == b); }
template <typename T>
bool operator<(const ExtendedArrayStack<T>& a, const ExtendedArrayStack<T>& b)
{ return a.to_vector() < b.to_vector(); }
template <typename T>
bool operator<=(const ExtendedArrayStack<T>& a, const ExtendedArrayStack<T>& b)
{ return !(b < a); }
template <typename T>
bool operator>(const ExtendedArrayStack<T>& a, const ExtendedArrayStack<T>& b)
{ return b < a; }
template <typename T>
bool operator>=(const ExtendedArrayStack<T>& a, const ExtendedArrayStack<T>& b)
{ return !(a < b); }

template <typename T>
bool operator==(const ExtendedLinkedStack<T>& a, const ExtendedLinkedStack<T>& b)
{ return a.to_vector() == b.to_vector(); }
template <typename T>
bool operator!=(const ExtendedLinkedStack<T>& a, const ExtendedLinkedStack<T>& b)
{ return !(a == b); }
template <typename T>
bool operator<(const ExtendedLinkedStack<T>& a, const ExtendedLinkedStack<T>& b)
{ return a.to_vector() < b.to_vector(); }
template <typename T>
bool operator<=(const ExtendedLinkedStack<T>& a, const ExtendedLinkedStack<T>& b)
{ return !(b < a); }
template <typename T>
bool operator>(const ExtendedLinkedStack<T>& a, const ExtendedLinkedStack<T>& b)
{ return b < a; }
template <typename T>
bool operator>=(const ExtendedLinkedStack<T>& a, const ExtendedLinkedStack<T>& b)
{ return !(a < b); }

} // namespace dsaac

template <typename T>
using ExtendedArrayStack = dsaac::ExtendedArrayStack<T>;
template <typename T>
using ExtendedLinkedStack = dsaac::ExtendedLinkedStack<T>;

#endif