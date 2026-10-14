#ifndef DSAAC_LINKED_LIST_HPP
#define DSAAC_LINKED_LIST_HPP

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace dsaac {

/*
 * 模板链表类：支持任意数据类型的存储。
 * 采用“头节点代称整表”的裸节点版本不同，这里封装为 RAII 类，
 * 由构造、拷贝、析构统一管理节点内存，调用方无须手动释放。
 */
template <typename T>
class LinkedList {
public:
    /* 链表节点结构体（T 为任意元素类型） */
    struct Node {
        T val;         // 节点值
        Node *next;    // 指向下一节点的指针
        Node(const T &x) : val(x), next(nullptr) {}
    };

    LinkedList();
    LinkedList(const LinkedList &other);            // 深拷贝
    LinkedList &operator=(const LinkedList &other); // 深拷贝赋值
    ~LinkedList();                                   // 释放全部节点

    /* ---------- 基本操作 ---------- */

    std::size_t size() const;      // 元素个数
    bool empty() const;            // 是否为空
    void push_front(const T &x);   // 头插（O(1)）
    void push_back(const T &x);    // 尾插（O(1)，维护了尾指针）
    void insert(std::size_t i, const T &x);  // 在下标 i 处插入，越界抛异常
    void erase(std::size_t i);               // 删除下标 i 处元素，越界抛异常
    T &at(std::size_t i);                   // 访问下标 i，越界抛 out_of_range
    const T &at(std::size_t i) const;
    int find(const T &x) const;     // 查找首个值为 x 的下标，不存在返回 -1
    bool contains(const T &x) const;
    void clear();                   // 清空并释放全部节点

    /* ---------- 扩展操作 ---------- */

    void reverse();                 // 就地反转（O(n) / O(1)）
    std::vector<T> to_vector() const;  // 导出元素序列
    T &front();                     // 访问首元素（空表抛异常）
    T &back();                      // 访问尾元素（空表抛异常）

    /* 内部节点访问入口：供进阶算法（任务 3）复用裸节点操作 */
    Node *head_node() { return head_; }
    const Node *head_node() const { return head_; }

private:
    Node *head_;   // 头节点指针（链表代称）
    Node *tail_;   // 尾节点指针（使 push_back 为 O(1)）
    std::size_t size_;  // 元素个数
};

} // namespace dsaac

template <typename T>
using LinkedList = dsaac::LinkedList<T>;

#endif // DSAAC_LINKED_LIST_HPP
