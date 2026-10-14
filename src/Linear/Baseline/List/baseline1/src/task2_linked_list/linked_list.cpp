#include "linked_list.hpp"

/* START: linked_list implementation */

/* 构造：空链表 */
template <typename T>
dsaac::LinkedList<T>::LinkedList()
{
    head_ = nullptr;
    tail_ = nullptr;
    size_ = 0;
}

/* 拷贝构造：逐节点深拷贝，两个链表互不影响 */
template <typename T>
dsaac::LinkedList<T>::LinkedList(const LinkedList &other)
{
    head_ = nullptr;
    tail_ = nullptr;
    size_ = 0;
    for (Node *cur = other.head_; cur != nullptr; cur = cur->next) {
        push_back(cur->val);  // 复用尾插，同时维护 tail_
    }
}

/* 拷贝赋值：先清空自身，再深拷贝（自赋值安全） */
template <typename T>
dsaac::LinkedList<T> &dsaac::LinkedList<T>::operator=(const LinkedList &other)
{
    if (this != &other) {
        clear();
        for (Node *cur = other.head_; cur != nullptr; cur = cur->next) {
            push_back(cur->val);
        }
    }
    return *this;
}

/* 析构：释放全部节点 */
template <typename T>
dsaac::LinkedList<T>::~LinkedList()
{
    clear();
}

template <typename T>
std::size_t dsaac::LinkedList<T>::size() const
{
    return size_;
}

template <typename T>
bool dsaac::LinkedList<T>::empty() const
{
    return size_ == 0;
}

/* 头插：新节点成为头节点 */
template <typename T>
void dsaac::LinkedList<T>::push_front(const T &x)
{
    Node *node = new Node(x);
    node->next = head_;
    head_ = node;
    if (tail_ == nullptr)  // 原表为空时，新节点既是头也是尾
        tail_ = node;
    size_++;
}

/* 尾插：借助 tail_ 指针实现 O(1) 追加 */
template <typename T>
void dsaac::LinkedList<T>::push_back(const T &x)
{
    Node *node = new Node(x);
    if (tail_ == nullptr) {
        head_ = node;
        tail_ = node;
    } else {
        tail_->next = node;
        tail_ = node;
    }
    size_++;
}

/* 在下标 i 处插入：i 合法范围 [0, size()]（i == size() 等价尾插） */
template <typename T>
void dsaac::LinkedList<T>::insert(std::size_t i, const T &x)
{
    if (i > size_)
        throw std::out_of_range("insert index out of range");
    if (i == 0) {
        push_front(x);
        return;
    }
    if (i == size_) {
        push_back(x);
        return;
    }
    // 找到下标 i-1 的前驱节点
    Node *prev = head_;
    for (std::size_t k = 0; k + 1 < i; ++k)
        prev = prev->next;
    Node *node = new Node(x);
    node->next = prev->next;
    prev->next = node;
    size_++;
}

/* 删除下标 i 处元素：负责释放节点内存 */
template <typename T>
void dsaac::LinkedList<T>::erase(std::size_t i)
{
    if (i >= size_)
        throw std::out_of_range("erase index out of range");
    Node *removed;
    if (i == 0) {
        removed = head_;
        head_ = head_->next;
        if (head_ == nullptr)  // 删空后尾指针同步置空
            tail_ = nullptr;
    } else {
        Node *prev = head_;
        for (std::size_t k = 0; k + 1 < i; ++k)
            prev = prev->next;
        removed = prev->next;
        prev->next = removed->next;
        if (removed == tail_)  // 删除的是尾节点时更新 tail_
            tail_ = prev;
    }
    delete removed;  // 释放内存
    size_--;
}

/* 访问下标 i：越界抛出异常，与 std::vector::at 行为一致 */
template <typename T>
T &dsaac::LinkedList<T>::at(std::size_t i)
{
    if (i >= size_)
        throw std::out_of_range("at index out of range");
    Node *cur = head_;
    for (std::size_t k = 0; k < i; ++k)
        cur = cur->next;
    return cur->val;
}

template <typename T>
const T &dsaac::LinkedList<T>::at(std::size_t i) const
{
    if (i >= size_)
        throw std::out_of_range("at index out of range");
    Node *cur = head_;
    for (std::size_t k = 0; k < i; ++k)
        cur = cur->next;
    return cur->val;
}

/* 查找首个值为 x 的下标，不存在返回 -1 */
template <typename T>
int dsaac::LinkedList<T>::find(const T &x) const
{
    int index = 0;
    for (Node *cur = head_; cur != nullptr; cur = cur->next) {
        if (cur->val == x)
            return index;
        index++;
    }
    return -1;
}

template <typename T>
bool dsaac::LinkedList<T>::contains(const T &x) const
{
    return find(x) != -1;
}

/* 清空链表：逐节点释放 */
template <typename T>
void dsaac::LinkedList<T>::clear()
{
    while (head_ != nullptr) {
        Node *next = head_->next;
        delete head_;
        head_ = next;
    }
    tail_ = nullptr;
    size_ = 0;
}

/* 就地反转：三指针迭代翻转，O(n) 时间 / O(1) 额外空间 */
template <typename T>
void dsaac::LinkedList<T>::reverse()
{
    Node *prev = nullptr;
    Node *cur = head_;
    while (cur != nullptr) {
        Node *next = cur->next;  // 暂存后继，防止断链
        cur->next = prev;        // 指针掉头
        prev = cur;              // prev 前进
        cur = next;              // cur 前进
    }
    tail_ = head_;  // 原头变尾
    head_ = prev;   // 原尾变头
}

/* 导出元素序列 */
template <typename T>
std::vector<T> dsaac::LinkedList<T>::to_vector() const
{
    std::vector<T> values;
    values.reserve(size_);
    for (Node *cur = head_; cur != nullptr; cur = cur->next)
        values.push_back(cur->val);
    return values;
}

/* 访问首元素 */
template <typename T>
T &dsaac::LinkedList<T>::front()
{
    if (empty())
        throw std::out_of_range("front on empty list");
    return head_->val;
}

/* 访问尾元素 */
template <typename T>
T &dsaac::LinkedList<T>::back()
{
    if (empty())
        throw std::out_of_range("back on empty list");
    return tail_->val;
}

/* 显式实例化常用类型，保证 list.cpp 与测试程序分开编译时链接通过 */
template class dsaac::LinkedList<int>;
template class dsaac::LinkedList<double>;
template class dsaac::LinkedList<char>;

/* END */
