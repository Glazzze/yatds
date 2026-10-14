#include "list.hpp"

/* START: basic_list implementation */

/* 在链表的节点 n0 之后插入节点 P */
void dsaac::insert(ListNode *n0, ListNode *P)
{
    ListNode *n1 = n0->next;
    P->next = n1;
    n0->next = P;
}

/* 删除链表的节点 n0 之后的首个节点 */
void dsaac::remove(ListNode *n0)
{
    if (n0->next == nullptr)
        return;
    // n0 -> P -> n1
    ListNode *P = n0->next;
    ListNode *n1 = P->next;
    n0->next = n1;
    // 释放内存
    delete P;
}

/* 访问链表中索引为 index 的节点 */
ListNode *dsaac::access(ListNode *head, int index)
{
    for (int i = 0; i < index; i++) {
        if (head == nullptr)
            return nullptr;
        head = head->next;
    }
    return head;
}

/* 在链表中查找值为 target 的首个节点 */
int dsaac::find(ListNode *head, int target)
{
    int index = 0;
    while (head != nullptr) {
        if (head->val == target)
            return index;
        head = head->next;
        index++;
    }
    return -1;
}

/* 按给定序列初始化链表（尾插法建表） */
ListNode *dsaac::build_list(const std::vector<int> &values)
{
    ListNode *head = nullptr;
    ListNode *tail = nullptr;
    for (int v : values) {
        ListNode *node = new ListNode(v);
        if (tail == nullptr) {
            // 空表时头、尾均指向首个节点
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }
    return head;
}

/* 头插：新节点成为头节点 */
ListNode *dsaac::push_front(ListNode *head, int val)
{
    ListNode *node = new ListNode(val);
    node->next = head;
    return node;
}

/* 尾插：追加到链表末尾（维护局部尾指针避免反复遍历） */
ListNode *dsaac::push_back(ListNode *head, int val)
{
    ListNode *node = new ListNode(val);
    if (head == nullptr)
        return node;
    ListNode *cur = head;
    while (cur->next != nullptr)
        cur = cur->next;
    cur->next = node;
    return head;
}

/* 求链表长度 */
int dsaac::length(ListNode *head)
{
    int n = 0;
    while (head != nullptr) {
        n++;
        head = head->next;
    }
    return n;
}

/* 判断链表中是否存在值为 target 的节点 */
bool dsaac::contains(ListNode *head, int target)
{
    return find(head, target) != -1;
}

/* 就地反转链表：prev/cur/next 三指针迭代翻转，不借助额外数组 */
ListNode *dsaac::reverse(ListNode *head)
{
    ListNode *prev = nullptr;
    ListNode *cur = head;
    while (cur != nullptr) {
        ListNode *next = cur->next;  // 暂存后继，防止断链
        cur->next = prev;            // 指针掉头
        prev = cur;                  // prev 前进
        cur = next;                  // cur 前进
    }
    return prev;  // 循环结束时 prev 即原尾节点、新头节点
}

/* 导出链表元素序列 */
std::vector<int> dsaac::to_vector(ListNode *head)
{
    std::vector<int> values;
    while (head != nullptr) {
        values.push_back(head->val);
        head = head->next;
    }
    return values;
}

/* 释放整条链表的全部节点 */
void dsaac::free_memory(ListNode *head)
{
    while (head != nullptr) {
        ListNode *next = head->next;
        delete head;
        head = next;
    }
}

/* END */
