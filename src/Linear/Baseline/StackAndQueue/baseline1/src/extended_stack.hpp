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

private:
    std::forward_list<T> values_;
};

} // namespace dsaac

template <typename T>
using ExtendedArrayStack = dsaac::ExtendedArrayStack<T>;
template <typename T>
using ExtendedLinkedStack = dsaac::ExtendedLinkedStack<T>;

#endif