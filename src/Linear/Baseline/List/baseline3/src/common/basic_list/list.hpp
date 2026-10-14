#ifndef DSAAC_LIST_HPP
#define DSAAC_LIST_HPP

#include <vector>

namespace dsaac {

/* 链表节点结构体 */
struct ListNode {
    int val;         // 节点值
    ListNode *next;  // 指向下一节点的指针
    ListNode(int x) : val(x), next(nullptr) {}  // 构造函数
};

/* ---------- 教程给出的四个基本操作 ---------- */

/* 在链表的节点 n0 之后插入节点 P（O(1)，只改写两条引用） */
void insert(ListNode *n0, ListNode *P);

/* 删除链表的节点 n0 之后的首个节点（O(1)，负责释放被删节点内存） */
void remove(ListNode *n0);

/* 访问链表中索引为 index 的节点（O(n)，从头顺序遍历，越界返回 nullptr） */
ListNode *access(ListNode *head, int index);

/* 在链表中查找值为 target 的首个节点（O(n)，返回下标，不存在返回 -1） */
int find(ListNode *head, int target);

/* ---------- 扩展操作 ---------- */

/* 按给定序列初始化链表（尾插法建表），返回头节点；空序列返回 nullptr */
ListNode *build_list(const std::vector<int> &values);

/* 头插：新节点成为头节点，返回新的头节点（O(1)） */
ListNode *push_front(ListNode *head, int val);

/* 尾插：追加到链表末尾，返回头节点（裸节点未维护尾指针，需 O(n) 遍历） */
ListNode *push_back(ListNode *head, int val);

/* 求链表长度（O(n)） */
int length(ListNode *head);

/* 判断链表中是否存在值为 target 的节点 */
bool contains(ListNode *head, int target);

/* 就地反转链表（迭代三指针，O(n) 时间 / O(1) 额外空间），返回新的头节点 */
ListNode *reverse(ListNode *head);

/* 导出链表元素序列，便于打印与比对 */
std::vector<int> to_vector(ListNode *head);

/* 释放整条链表的全部节点，防止内存泄漏 */
void free_memory(ListNode *head);

} // namespace dsaac

// 为了方便使用，将类型与函数暴露在全局（也可按需不暴露）
using dsaac::ListNode;
using dsaac::insert;
using dsaac::remove;
using dsaac::access;
using dsaac::find;
using dsaac::build_list;
using dsaac::push_front;
using dsaac::push_back;
using dsaac::length;
using dsaac::contains;
using dsaac::reverse;
using dsaac::to_vector;
using dsaac::free_memory;

#endif // DSAAC_LIST_HPP
