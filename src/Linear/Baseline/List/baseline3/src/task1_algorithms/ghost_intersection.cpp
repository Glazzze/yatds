#include "ghost_intersection.hpp"

#include "../common/basic_list/list.hpp"

/* START: ghost_intersection implementation */

/* 返回 head 到 inter 之前的最后一个结点；前缀为空（head 本身就是 inter）返回 nullptr */
static ListNode *prefixTail(ListNode *head, ListNode *inter)
{
    if (head == nullptr || head == inter)
        return nullptr;
    ListNode *c = head;
    while (c->next != nullptr && c->next != inter)
        c = c->next;
    return (c->next == inter) ? c : nullptr;
}

/* 释放 head 到 stop 之前的全部结点（不含 stop），用于释放两条链表的独有前缀 */
static void freePrefix(ListNode *head, ListNode *stop)
{
    ListNode *c = head;
    while (c != nullptr && c != stop) {
        ListNode *next = c->next;
        delete c;
        c = next;
    }
}

/* ---------- 挑战 1：探查环路 ---------- */

ListNode *GhostIntersection::detectAndRemoveCycle(ListNode *head)
{
    if (head == nullptr)
        return nullptr;

    /* 第一阶段：Floyd 快慢指针。快指针每次两步，慢指针一步，若有环必相遇。 */
    ListNode *slow = head;
    ListNode *fast = head;
    while (fast != nullptr && fast->next != nullptr) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            break;
    }
    /* fast 走到链尾说明无环 */
    if (fast == nullptr || fast->next == nullptr)
        return nullptr;

    /* 第二阶段：定位环入口。将 slow 拨回起点，两指针同速前进，相遇处即入口。 */
    slow = head;
    while (slow != fast) {
        slow = slow->next;
        fast = fast->next;
    }
    ListNode *entry = slow;

    /* 第三阶段：解环。从入口沿环前进，找到 next 指回入口的结点并断开。
       从 entry 起步（而非 head），步数受环长约束，保证有限步内终止；
       单结点自环、尾结点自环时循环体一次都不执行，同样正确。 */
    ListNode *p = entry;
    while (p->next != entry)
        p = p->next;
    p->next = nullptr;

    return entry;
}

/* ---------- 挑战 2：寻找交汇点 ---------- */

ListNode *GhostIntersection::getIntersectionNode(ListNode *headA, ListNode *headB)
{
    ListNode *pA = headA;
    ListNode *pB = headB;

    /* 两指针各走完全程后切换到对方链表的头部，使得走过的总步数相等（lenA+lenB），
       若相交必在交汇点相遇，不相交则同时变为 nullptr。
       注意：必须写成 pA = pA ? pA->next : headB，使比较能看到 nullptr；
       常见的 "pA = pA->next; if (!pA) pA = headB;" 写法会让比较永远看不到 nullptr，
       两链表不相交时陷入死循环。 */
    while (pA != pB) {
        pA = pA ? pA->next : headB;
        pB = pB ? pB->next : headA;
    }
    return pA;
}

/* ---------- 挑战 3：空间逆转 ---------- */

ListNode *GhostIntersection::reverseSharedPath(ListNode *&headA, ListNode *&headB, ListNode *intersection)
{
    if (intersection == nullptr)
        return nullptr;

    /* 关键：必须在反转“之前”捕获两条前缀的尾结点。
       反转会把 intersection 的 next 置空并改变共享段方向，若反转后再沿前缀遍历，
       遍历会越过原前缀尾、直接走进已反转的共享段，重新接线时闭合成环，
       导致后续遍历与释放永久挂死。 */
    ListNode *tailA = prefixTail(headA, intersection);
    ListNode *tailB = prefixTail(headB, intersection);

    /* 反转共享段：three-pointer 就地反转，返回原共享尾结点（新头） */
    ListNode *newHead = dsaac::reverse(intersection);

    /* 将各自前缀（或表头）重新接到反转后的共享段新头 */
    if (tailA != nullptr)
        tailA->next = newHead;
    else
        headA = newHead; // 前缀为空：headA 原本就是 intersection，其头需更新

    if (tailB != nullptr)
        tailB->next = newHead;
    else
        headB = newHead;

    return newHead;
}

/* ---------- Scenario：RAII 生命周期管理 ---------- */

Scenario::~Scenario()
{
    /* 1. 先解环：对仍成环的链表直接释放会死循环 */
    GhostIntersection::detectAndRemoveCycle(headA);
    GhostIntersection::detectAndRemoveCycle(headB);

    /* 2. 再释放：相交链表共享尾部，两个头都释放会双重释放 */
    if (boundary != nullptr) {
        freePrefix(headA, boundary); // 释放 ListA 独有前缀
        freePrefix(headB, boundary); // 释放 ListB 独有前缀
        dsaac::free_memory(boundary); // 公共部分只释放一次
    } else {
        dsaac::free_memory(headA); // 两条独立链表各自释放
        dsaac::free_memory(headB);
    }
}

void Scenario::reverseShared()
{
    if (boundary == nullptr)
        return;
    /* 反转公共部分并同步 boundary：反转后 boundary 变为共享段新头，
       维持析构时“按当前共享头释放一次”的不变量 */
    boundary = GhostIntersection::reverseSharedPath(headA, headB, boundary);
}

/* ---------- 构造辅助 ---------- */

ListNode *build_cycle(const std::vector<int> &values, int entry)
{
    ListNode *head = dsaac::build_list(values);
    if (head != nullptr && entry >= 0 && entry < static_cast<int>(values.size())) {
        ListNode *tail = dsaac::access(head, static_cast<int>(values.size()) - 1);
        ListNode *entryNode = dsaac::access(head, entry);
        if (tail != nullptr && entryNode != nullptr)
            tail->next = entryNode; // 尾结点指回入口，形成环
    }
    return head;
}

ListNode *build_shared(const std::vector<int> &prefixA, const std::vector<int> &prefixB,
                       const std::vector<int> &shared, ListNode *&headA, ListNode *&headB)
{
    ListNode *sharedHead = dsaac::build_list(shared);

    /* ListA = prefixA + shared */
    if (prefixA.empty()) {
        headA = sharedHead; // 空前缀：头即共享段头
    } else {
        headA = dsaac::build_list(prefixA);
        ListNode *tailA = dsaac::access(headA, static_cast<int>(prefixA.size()) - 1);
        tailA->next = sharedHead;
    }

    /* ListB = prefixB + shared */
    if (prefixB.empty()) {
        headB = sharedHead;
    } else {
        headB = dsaac::build_list(prefixB);
        ListNode *tailB = dsaac::access(headB, static_cast<int>(prefixB.size()) - 1);
        tailB->next = sharedHead;
    }

    return sharedHead; // 公共部分头结点（boundary）
}

/* END */