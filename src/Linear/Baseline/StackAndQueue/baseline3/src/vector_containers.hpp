#ifndef DSAAC_VECTOR_CONTAINERS_HPP
#define DSAAC_VECTOR_CONTAINERS_HPP

#include <cstddef>
#include <vector>

namespace dsaac {

/*
 * 用 <vector> 实现的栈与队列，各带一个互转函数：
 *   VectorStack::to_queue()  把栈转换为队列（栈顶→队首）
 *   VectorQueue::to_stack()  把队列转换为栈（队首→栈底）
 * 两个类放在同一头文件，避免互转函数的循环依赖。
 */

template <typename T>
class VectorQueue;  // 前置声明

template <typename T>
class VectorStack {
public:
    bool empty() const;
    std::size_t size() const;
    void push(const T& value);
    void pop();
    const T& top() const;

    VectorQueue<T> to_queue() const;  // 转换为队列

private:
    std::vector<T> data_;
};

template <typename T>
class VectorQueue {
public:
    bool empty() const;
    std::size_t size() const;
    void push(const T& value);
    void pop();
    const T& front() const;

    VectorStack<T> to_stack() const;  // 转换为栈

private:
    std::vector<T> data_;
    std::size_t head_ = 0;  // 队首下标（出队即 head_ 前进，避免头部删除的 O(n)）
};

} // namespace dsaac

template <typename T>
using VectorStack = dsaac::VectorStack<T>;
template <typename T>
using VectorQueue = dsaac::VectorQueue<T>;

#endif