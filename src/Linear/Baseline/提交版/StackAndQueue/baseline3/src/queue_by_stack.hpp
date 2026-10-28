#ifndef DSAAC_QUEUE_BY_STACK_HPP
#define DSAAC_QUEUE_BY_STACK_HPP

#include <cstddef>
#include <stack>

namespace dsaac {

/*
 * 用栈实现队列（双栈法）。
 *
 * 思路：in_ 负责入队（push 到栈顶）；out_ 负责出队。出队或访问队首时，
 * 若 out_ 为空，就把 in_ 的元素全部倒入 out_（顺序被反转两次，队首来到
 * out_ 栈顶）。每个元素至多被移动一次，均摊 O(1)。
 * 两个栈都声明为 mutable，使 const 的 front()/empty() 也能触发倒栈。
 */
template <typename T>
class QueueByStack {
public:
    bool empty() const;
    std::size_t size() const;
    void push(const T& value);
    void pop();
    const T& front() const;

private:
    void transfer() const;  // 把 in_ 倒入 out_
    mutable std::stack<T> in_;
    mutable std::stack<T> out_;
};

} // namespace dsaac

template <typename T>
using QueueByStack = dsaac::QueueByStack<T>;

#endif