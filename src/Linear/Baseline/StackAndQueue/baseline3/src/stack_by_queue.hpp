#ifndef DSAAC_STACK_BY_QUEUE_HPP
#define DSAAC_STACK_BY_QUEUE_HPP

#include <cstddef>
#include <queue>

namespace dsaac {

/*
 * 用队列实现栈。
 *
 * 思路：入栈即把元素加入队尾（O(1)）；栈顶就是最后入队的元素，即队尾
 * （std::queue::back()，O(1)）；出栈时把队首元素依次"旋转"到队尾 n-1 次，
 * 使原队尾（栈顶）来到队首，再出队（O(n)）。
 * 这样只用一个队列即可，无需双队列。
 */
template <typename T>
class StackByQueue {
public:
    bool empty() const;
    std::size_t size() const;
    void push(const T& value);
    void pop();
    const T& top() const;

private:
    std::queue<T> q_;
};

} // namespace dsaac

template <typename T>
using StackByQueue = dsaac::StackByQueue<T>;

#endif