#ifndef DSAAC_GHOST_INTERSECTION_HPP
#define DSAAC_GHOST_INTERSECTION_HPP

/*
 * Baseline 任务 3「幽灵交叉路口」的核心算法模块。
 *
 * 三个算法均满足 O(n) 时间 / O(1) 额外空间（不使用哈希表或额外数组记录节点），
 * 且一律用迭代实现（递归会引入 O(n) 栈空间）。
 */

#include "../common/basic_list/list.hpp"

#include <vector>

/*
 * 纯静态求解类：三个挑战分别对应一个静态方法。
 * 命名沿用 README 指定的 camelCase（detectAndRemoveCycle 等）。
 */
class GhostIntersection {
public:
    /*
     * 挑战 1：探查环路。
     * Floyd 快慢指针判环，若存在环则定位环的入口结点并就地解除环状结构。
     * 返回环入口结点；无环返回 nullptr。
     * 时间 O(n)，空间 O(1)。
     */
    static ListNode *detectAndRemoveCycle(ListNode *head);

    /*
     * 挑战 2：寻找交汇点。
     * 双指针法：pA 走完 ListA 转走 ListB，pB 走完 ListB 转走 ListA，
     * 二者必然在交汇点相遇（按“指针身份”比较，而非结点值）。
     * 返回开始交汇的结点；不相交返回 nullptr。要求两链表均无环。
     * 时间 O(lenA + lenB)，空间 O(1)。
     */
    static ListNode *getIntersectionNode(ListNode *headA, ListNode *headB);

    /*
     * 挑战 3：空间逆转。
     * 仅就地反转两链表交汇后的公共部分，各自独立的头部前缀不被破坏。
     * 因公共部分被两条链表共享，head 需按引用传入（前缀为空时头结点会改变）。
     * 返回反转后公共部分的新头结点（原公共尾结点）。
     * 时间 O(n)，空间 O(1)。
     */
    static ListNode *reverseSharedPath(ListNode *&headA, ListNode *&headB, ListNode *intersection);
};

/*
 * RAII 场景持有者：统一管理一条（或两条）测试链表的生命周期，
 * 析构时按正确规则释放，杜绝两类常见泄漏/错误：
 *   1. 对仍成环的链表直接释放会死循环 —— 故先解环再释放；
 *   2. 相交链表两个头都释放会双重释放共享尾部 —— 故各自前缀释放到 boundary
 *      为止，公共部分只释放一次。
 */
struct Scenario {
    ListNode *headA = nullptr;     // 第 1 条链表头
    ListNode *headB = nullptr;     // 第 2 条链表头（单链表场景为 nullptr）
    ListNode *boundary = nullptr;  // 公共部分当前头结点；无公共部分为 nullptr

    Scenario() = default;
    Scenario(const Scenario &) = delete;             // 持有裸资源，禁止拷贝
    Scenario &operator=(const Scenario &) = delete;
    ~Scenario();

    /* 反转公共部分，并同步更新 boundary 以维持析构不变量 */
    void reverseShared();
};

/* ---------- 构造辅助 ---------- */

/*
 * 构造一条（可能成环的）链表：按 values 顺序建结点，
 * 若 entry >= 0 且 entry < n，则令尾结点的 next 指向下标 entry 的结点。
 * entry < 0 表示无环。返回头结点。
 */
ListNode *build_cycle(const std::vector<int> &values, int entry);

/*
 * 构造两条共享同一段尾部（物理同一批结点）的链表：
 *   ListA = prefixA + shared，ListB = prefixB + shared。
 * 前缀可以为空（此时对应 head 直接等于共享链头）。
 * 通过出参返回两条链表的头，并返回公共部分头结点（供 Scenario::boundary 使用）。
 */
ListNode *build_shared(const std::vector<int> &prefixA, const std::vector<int> &prefixB,
                       const std::vector<int> &shared, ListNode *&headA, ListNode *&headB);

#endif // DSAAC_GHOST_INTERSECTION_HPP