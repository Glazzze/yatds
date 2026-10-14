/*
 * Baseline 任务 3 · 手动构造测试程序（满足任务内容第 2 条）
 *
 * 在测试代码中“手动”构造三类边界链表，并分步打印命令行可视化输出、
 * 对每一步断言，验证三个挑战的正确性：
 *   1. 存在环的单条测试链表
 *   2. 不相交的两条独立无环链表
 *   3. 相交的两条无环链表
 *
 * 每个场景用 RAII 的 Scenario 持有链表，析构时按正确规则释放（先解环、
 * 共享尾部只释放一次），配合 `make asan` 可验证零泄漏、无双重释放。
 */

#include "../task1_algorithms/ghost_intersection.hpp"
#include "../common/basic_list/list.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/* 已通过 / 失败的检查项数量 */
static int gPassed = 0;
static int gFailed = 0;

/* 断言辅助：比较链表内容与期望序列 */
static void expect(std::vector<int> actual, std::vector<int> expected, const std::string &name)
{
    if (actual == expected) {
        gPassed++;
        std::cout << "[PASS] " << name << '\n';
    } else {
        gFailed++;
        std::cout << "[FAIL] " << name << "  期望:";
        for (int x : expected)
            std::cout << ' ' << x;
        std::cout << "  实际:";
        for (int x : actual)
            std::cout << ' ' << x;
        std::cout << '\n';
    }
}

/* 可视化：把链表打印成 "a -> b -> c -> null"（或带环提示） */
static void visualize(const std::string &label, ListNode *head)
{
    std::cout << "  " << label << ": ";
    std::vector<int> values = dsaac::to_vector(head);
    if (values.empty()) {
        std::cout << "EMPTY";
    } else {
        for (std::size_t i = 0; i < values.size(); i++)
            std::cout << values[i] << " -> ";
        std::cout << "null";
    }
    std::cout << '\n';
}

/* 可视化含环链表：只走限定步数，避免沿环无限遍历 */
static void visualizeCyclic(const std::string &label, ListNode *head, int steps)
{
    std::cout << "  " << label << ": ";
    ListNode *c = head;
    for (int i = 0; i < steps && c != nullptr; i++) {
        std::cout << c->val << " -> ";
        c = c->next;
    }
    std::cout << (c != nullptr ? "(环) ..." : "null") << '\n';
}

/* ---------- 场景 1：存在环的单条链表 ---------- */

static void testCycle()
{
    std::cout << "== 场景 1：存在环的单条链表 ==\n";
    /* 手动构造 1 -> 2 -> 3 -> 4 -> 5，并令尾结点指回下标 2 的结点（值为 3） */
    Scenario sc;
    sc.headA = build_cycle({1, 2, 3, 4, 5}, 2);
    /* 此时链表成环，不能用 to_vector（会无限遍历），改用限步可视化 */
    visualizeCyclic("构造后的链表（含环）", sc.headA, 8);

    ListNode *entry = GhostIntersection::detectAndRemoveCycle(sc.headA);
    std::cout << "  环入口值: " << (entry ? std::to_string(entry->val) : "NONE") << '\n';
    visualize("解环后", sc.headA);

    expect(entry != nullptr ? std::vector<int>{entry->val} : std::vector<int>{}, {3},
           "环入口值为 3");
    expect(dsaac::to_vector(sc.headA), {1, 2, 3, 4, 5}, "解环后链表为 1 2 3 4 5");
}

/* ---------- 场景 2：不相交的两条独立无环链表 ---------- */

static void testNoIntersection()
{
    std::cout << "== 场景 2：不相交的两条无环链表 ==\n";
    Scenario sc;
    sc.headA = dsaac::build_list({1, 2, 3});
    sc.headB = dsaac::build_list({4, 5});
    visualize("ListA", sc.headA);
    visualize("ListB", sc.headB);

    ListNode *inter = GhostIntersection::getIntersectionNode(sc.headA, sc.headB);
    std::cout << "  交汇点: " << (inter ? std::to_string(inter->val) : "NONE") << '\n';
    expect(inter == nullptr ? std::vector<int>{} : std::vector<int>{inter->val}, {},
           "不相交，交汇点为 NONE");
}

/* ---------- 场景 3：相交的两条无环链表 ---------- */

static void testIntersection()
{
    std::cout << "== 场景 3：相交的两条无环链表 ==\n";
    /* 手动构造共享尾部 7 -> 8 -> 9，A 前缀 10 20，B 前缀 30 */
    Scenario sc;
    sc.boundary = build_shared({10, 20}, {30}, {7, 8, 9}, sc.headA, sc.headB);
    visualize("ListA（含共享尾部）", sc.headA);
    visualize("ListB（含共享尾部）", sc.headB);

    ListNode *inter = GhostIntersection::getIntersectionNode(sc.headA, sc.headB);
    std::cout << "  交汇点值: " << (inter ? std::to_string(inter->val) : "NONE") << '\n';
    expect(inter != nullptr ? std::vector<int>{inter->val} : std::vector<int>{}, {7},
           "交汇点值为 7");

    sc.reverseShared();
    visualize("反转公共部分后的 ListA", sc.headA);
    visualize("反转公共部分后的 ListB", sc.headB);
    expect(dsaac::to_vector(sc.headA), {10, 20, 9, 8, 7}, "ListA 公共部分反转正确");
    expect(dsaac::to_vector(sc.headB), {30, 9, 8, 7}, "ListB 公共部分反转正确");
}

int main()
{
    std::cout << "===== 幽灵交叉路口：手动构造测试 =====\n\n";
    testCycle();
    std::cout << '\n';
    testNoIntersection();
    std::cout << '\n';
    testIntersection();
    std::cout << "\n合计：通过 " << gPassed << " 项，失败 " << gFailed << " 项\n";
    return gFailed > 0 ? 1 : 0;
}