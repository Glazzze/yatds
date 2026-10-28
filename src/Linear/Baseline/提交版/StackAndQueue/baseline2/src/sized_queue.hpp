#ifndef DSAAC_SIZED_QUEUE_HPP
#define DSAAC_SIZED_QUEUE_HPP

#include <cstddef>
#include <vector>

namespace dsaac {

/*
 * 循环队列的第二种实现（对照给定骨架的 CircularQueue）。
 *
 * 给定 CircularQueue 用"多分配一个空槽"来区分空/满（牺牲一格永不存数据）。
 * 本实现改用父级 README 正文给出的思路：front 记录队首下标，size 记录元素
 * 个数，令 rear = front + size（取模回绕）。这样无需牺牲空槽，
 * 容量为 capacity 的队列可真正存满 capacity 个元素——与空槽法形成对照。
 */
template <typename T>
class SizedQueue {
public:
    explicit SizedQueue(std::size_t capacity);

    bool empty() const;
    bool full() const;
    std::size_t size() const;
    void push(const T& value);
    void pop();
    const T& front() const;

private:
    std::vector<T> buffer_;       // 恰好 capacity 个槽，无浪费
    std::size_t front_ = 0;       // 队首下标
    std::size_t count_ = 0;       // 元素个数；rear = front_ + count_（取模）
};

} // namespace dsaac

template <typename T>
using SizedQueue = dsaac::SizedQueue<T>;

#endif