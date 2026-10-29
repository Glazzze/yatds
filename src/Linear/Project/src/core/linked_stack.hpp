#ifndef ZUMA_CORE_LINKED_STACK_HPP
#define ZUMA_CORE_LINKED_STACK_HPP

#include <stdexcept>
#include <utility>

namespace zuma {

/* 自包含的单向链式栈：头节点为栈顶，不设容量上限。
 * push 在节点构造成功后才更新栈顶；pop/top 在空栈时抛异常。
 * 链接操作为 O(1)，push 的总成本还包含 T 的复制或移动成本。
 * 禁止复制，避免节点被多个容器拥有；析构迭代释放，避免深栈递归。 */
template <typename T>
class LinkedStack {
public:
    LinkedStack() = default;
    LinkedStack(const LinkedStack &) = delete;
    LinkedStack &operator=(const LinkedStack &) = delete;

    ~LinkedStack()
    {
        while (head_) {
            Node *old = head_;
            head_ = head_->next;
            delete old;
        }
    }

    bool empty() const { return head_ == nullptr; }

    void push(const T &value) { head_ = new Node(value, head_); }
    void push(T &&value) { head_ = new Node(std::move(value), head_); }

    T &top()
    {
        if (empty())
            throw std::out_of_range("撤销栈为空");
        return head_->value;
    }

    const T &top() const
    {
        if (empty())
            throw std::out_of_range("撤销栈为空");
        return head_->value;
    }

    void pop()
    {
        if (empty())
            throw std::out_of_range("撤销栈为空");
        Node *old = head_;
        head_ = head_->next;
        delete old;
    }

private:
    struct Node {
        T value;
        Node *next;

        Node(const T &v, Node *n) : value(v), next(n) {}
        Node(T &&v, Node *n) : value(std::move(v)), next(n) {}
    };

    Node *head_ = nullptr;
};

} // namespace zuma

#endif // ZUMA_CORE_LINKED_STACK_HPP
