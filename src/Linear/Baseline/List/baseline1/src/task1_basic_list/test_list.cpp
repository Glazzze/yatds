/*
 * 小任务 1 测试程序：验证链表基本实现的正确性。
 *
 * 测试设计说明（不只是简单功能测试）：
 * - 覆盖单节点、空表等边界情况；
 * - 删除操作后用 asan 无内存泄漏的运行结果证明内存管理正确；
 * - insert/remove/access/find 的“位置语义”互相交叉验证：
 *   例如先 insert 再 access，检查被断言的正是刚插入的节点；
 * - 反转操作验证头节点与尾节点这两个最易出错的边界指针。
 */
#include "list.hpp"

#include <iostream>
#include <vector>

/* 断言辅助：统一打印通过/失败信息，便于人工核对 */
static void check(const char *name, bool ok)
{
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << '\n';
}

/* 辅助：将序列打印成一行 */
static void print_values(const std::vector<int> &values)
{
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) std::cout << ' ';
        std::cout << values[i];
    }
    std::cout << '\n';
}

int main()
{
    /* ---------- 1. 教程链表 1 -> 3 -> 2 -> 5 -> 4 ---------- */
    std::cout << "== 1. 初始化与遍历 ==\n";
    ListNode *n0 = new ListNode(1);
    ListNode *n1 = new ListNode(3);
    ListNode *n2 = new ListNode(2);
    ListNode *n3 = new ListNode(5);
    ListNode *n4 = new ListNode(4);
    n0->next = n1;
    n1->next = n2;
    n2->next = n3;
    n3->next = n4;
    print_values(to_vector(n0));
    check("length == 5", length(n0) == 5);

    /* ---------- 2. 插入节点：n0 与 n1 之间插入 9 ---------- */
    std::cout << "== 2. 插入节点 ==\n";
    insert(n0, new ListNode(9));  // 期望 1 9 3 2 5 4
    print_values(to_vector(n0));
    check("插入后 access(1)->val == 9", access(n0, 1)->val == 9);
    check("插入后 length == 6", length(n0) == 6);

    /* ---------- 3. 删除节点：删除 n0 之后的首个节点（即 9） ---------- */
    std::cout << "== 3. 删除节点 ==\n";
    remove(n0);  // 期望还原为 1 3 2 5 4
    print_values(to_vector(n0));
    check("删除后 length == 5", length(n0) == 5);
    check("删除后 access(1)->val == 3", access(n0, 1)->val == 3);

    /* ---------- 4. 访问与越界 ---------- */
    std::cout << "== 4. 访问节点 ==\n";
    check("access(0) 是头节点", access(n0, 0) == n0);
    check("access(4) 是尾节点", access(n0, 4) == n4);
    check("access(5) 越界返回 nullptr", access(n0, 5) == nullptr);
    check("access(-1) 直接返回头节点（与教程实现一致）", access(n0, -1) == n0);

    /* ---------- 5. 查找 ---------- */
    std::cout << "== 5. 查找节点 ==\n";
    check("find(1) == 0（头节点）", find(n0, 1) == 0);
    check("find(4) == 4（尾节点）", find(n0, 4) == 4);
    check("find(100) == -1（不存在）", find(n0, 100) == -1);
    check("contains(3)", contains(n0, 3));
    check("!contains(9)", !contains(n0, 9));

    free_memory(n0);

    /* ---------- 6. 头插 / 尾插 / 建表 ---------- */
    std::cout << "== 6. 头插、尾插与建表 ==\n";
    ListNode *list = build_list({1, 2, 3});
    list = push_front(list, 0);  // 期望 0 1 2 3
    list = push_back(list, 4);   // 期望 0 1 2 3 4
    print_values(to_vector(list));
    check("头插+尾插后 length == 5", length(list) == 5);

    /* 空表尾插：返回的新头即唯一节点 */
    ListNode *single = push_back(nullptr, 42);
    check("空表尾插得到单节点表", length(single) == 1 && single->val == 42);
    free_memory(single);

    /* ---------- 7. 就地反转 ---------- */
    std::cout << "== 7. 反转链表 ==\n";
    list = reverse(list);  // 期望 4 3 2 1 0
    print_values(to_vector(list));
    check("反转后头节点值为 4", list->val == 4);
    check("反转后 access(4)->val == 0", access(list, 4)->val == 0);

    /* 空表反转：返回 nullptr，不崩溃 */
    check("空表反转返回 nullptr", reverse(nullptr) == nullptr);
    free_memory(list);

    /* 单节点反转：仍是自身 */
    ListNode *one = new ListNode(7);
    check("单节点反转后值不变", reverse(one)->val == 7);
    free_memory(one);

    std::cout << "全部小任务 1 测试完成。\n";
    return 0;
}
