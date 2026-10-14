#include "linked_queue.hpp"

#include <stdexcept>

/* START: linked_queue implementation */

template <typename T>
dsaac::LinkedQueue<T>::LinkedQueue()
    : front_(nullptr), rear_(nullptr), size_(0)
{
}

/* 析构：从头节点开始逐个 delete，避免内存泄漏 */
template <typename T>
dsaac::LinkedQueue<T>::~LinkedQueue()
{
    while (front_ != nullptr) {
        Node *next = front_->next;
        delete front_;
        front_ = next;
    }
}

template <typename T>
bool dsaac::LinkedQueue<T>::empty() const
{
    return size_ == 0;
}

template <typename T>
std::size_t dsaac::LinkedQueue<T>::size() const
{
    return size_;
}

/* 入队：在队尾追加节点（O(1)） */
template <typename T>
void dsaac::LinkedQueue<T>::push(const T& value)
{
    Node *node = new Node(value);
    if (rear_ == nullptr) {  // 空队列：头、尾都指向新节点
        front_ = node;
        rear_ = node;
    } else {
        rear_->next = node;
        rear_ = node;
    }
    size_++;
}

/* 出队：删除队首节点（O(1)） */
template <typename T>
void dsaac::LinkedQueue<T>::pop()
{
    if (empty())
        throw std::out_of_range("队列为空");
    Node *tmp = front_;
    front_ = front_->next;
    if (front_ == nullptr)  // 删除后队列为空，尾指针置空
        rear_ = nullptr;
    delete tmp;
    size_--;
}

/* 访问队首元素：只读引用，不出队 */
template <typename T>
const T& dsaac::LinkedQueue<T>::front() const
{
    if (empty())
        throw std::out_of_range("队列为空");
    return front_->val;
}

/* 显式实例化，保证与测试程序分开编译时链接通过 */
template class dsaac::LinkedQueue<int>;
template class dsaac::LinkedQueue<double>;
template class dsaac::LinkedQueue<char>;

/* END */