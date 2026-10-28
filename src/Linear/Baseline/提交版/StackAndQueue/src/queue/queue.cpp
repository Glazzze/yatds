#include "queue.hpp"

#include <stdexcept>

/* START: queue implementation */

/*
 * 循环队列实现：vector 作为环形缓冲区，front_ / back_ 两个下标推进。
 *
 * 关键技巧——多分配一个空槽（buffer_.size() == capacity + 1）来区分
 * "空"与"满"：
 *   - empty() 判据：front_ == back_
 *   - full()  判据：back_ 的下一格回绕后撞上前端，即 (back_ + 1) % size == front_
 * 牺牲的那一格永远不存数据，于是"空"和"满"不再同形。
 *
 * 下标推进到缓冲区末尾时用取模回绕到开头，这正是循环队列
 * 解决"front/rear 到达数组尾部无法继续移动"的核心手段。
 */

/* 构造函数：创建可容纳 capacity 个元素的环形缓冲区 */
template <typename T>
dsaac::CircularQueue<T>::CircularQueue(std::size_t capacity)
{
    // 多分配一个槽用于区分空/满；capacity=0 时 buffer 仍有 1 槽，
    // 此时 full() 与 empty() 同时成立，push 立即上溢，语义自洽
    buffer_.resize(capacity + 1);
    front_ = 0;
    back_ = 0;
}

/* 判断队列是否为空 */
template <typename T>
bool dsaac::CircularQueue<T>::empty() const
{
    return front_ == back_;
}

/* 判断队列是否已满：队尾的下一格回绕后正好是队首 */
template <typename T>
bool dsaac::CircularQueue<T>::full() const
{
    return (back_ + 1) % buffer_.size() == front_;
}

/* 入队：满队列抛上溢异常，否则在队尾写入并推进 back_（末尾回绕到 0） */
template <typename T>
void dsaac::CircularQueue<T>::push(const T& value)
{
    if (full())
        throw std::overflow_error("队列已满");
    buffer_[back_] = value;
    back_ = (back_ + 1) % buffer_.size();
}

/* 出队：空队列抛下溢异常，否则推进 front_（末尾回绕到 0），不返回值 */
template <typename T>
void dsaac::CircularQueue<T>::pop()
{
    if (empty())
        throw std::out_of_range("队列为空");
    front_ = (front_ + 1) % buffer_.size();
}

/* 访问队首元素：返回只读引用，不出队、不修改队列 */
template <typename T>
const T& dsaac::CircularQueue<T>::front() const
{
    if (empty())
        throw std::out_of_range("队列为空");
    return buffer_[front_];
}

/* 显式实例化常用类型，保证 queue.cpp 与测试程序分开编译时链接通过 */
template class dsaac::CircularQueue<int>;
template class dsaac::CircularQueue<double>;
template class dsaac::CircularQueue<char>;

/* END */