#ifndef DSAAC_LINKED_QUEUE_HPP
#define DSAAC_LINKED_QUEUE_HPP

#include <cstddef>

namespace dsaac {

/*
 * 链式队列：以单链表的头节点为队首、尾节点为队尾。
 * 队尾仅添加节点（O(1)），队首仅删除节点（O(1)）。
 * 与循环队列不同，链表按需分配，无容量上限。
 */
template <typename T>
class LinkedQueue {
public:
    LinkedQueue();
    ~LinkedQueue();

    LinkedQueue(const LinkedQueue&) = delete;             // 裸指针，禁止浅拷贝
    LinkedQueue& operator=(const LinkedQueue&) = delete;

    bool empty() const;
    std::size_t size() const;
    void push(const T& value);
    void pop();
    const T& front() const;

private:
    struct Node {
        T val;
        Node *next;
        Node(const T& v) : val(v), next(nullptr) {}
    };
    Node *front_;  // 队首
    Node *rear_;   // 队尾
    std::size_t size_;
};

} // namespace dsaac

template <typename T>
using LinkedQueue = dsaac::LinkedQueue<T>;

#endif