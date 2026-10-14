/*
 * 小任务 2 测试程序：验证模板链表类支持任意数据类型。
 *
 * 测试设计说明（不只是简单功能测试）：
 * - 分别用 int、double、char 三种类型实例化模板，证明“任意类型”特性；
 * - 深拷贝语义验证：修改副本后原表不受影响，防止浅拷贝共享节点；
 * - 自赋值安全验证；
 * - 异常语义验证：越界 at/insert/erase 抛出 std::out_of_range；
 * - 尾指针维护验证：反复头插、尾插、删除头/尾节点后 back() 仍正确。
 */
#include "linked_list.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/* 断言辅助 */
static void check(const char *name, bool ok)
{
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << '\n';
}

/* 辅助：序列转字符串，便于打印任意类型元素 */
template <typename T>
static std::string join(const std::vector<T> &values)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ' ';
        oss << values[i];
    }
    return oss.str();
}

int main()
{
    /* ---------- 1. int 类型：基础全流程 ---------- */
    std::cout << "== 1. int 实例化 ==\n";
    LinkedList<int> list;
    for (int i = 1; i <= 5; ++i)
        list.push_back(i);
    list.push_front(0);
    std::cout << join(list.to_vector()) << '\n';  // 期望 0 1 2 3 4 5
    check("size == 6", list.size() == 6);

    list.insert(2, 99);          // 期望 0 1 99 2 3 4 5
    check("insert(2, 99) 后 at(2) == 99", list.at(2) == 99);
    list.erase(2);               // 期望还原
    check("erase(2) 后 at(2) == 2", list.at(2) == 2);

    check("find(5) == 5", list.find(5) == 5);
    check("find(100) == -1", list.find(100) == -1);
    check("front() == 0", list.front() == 0);
    check("back() == 5", list.back() == 5);

    /* ---------- 2. 反转与清空 ---------- */
    std::cout << "== 2. 反转与清空 ==\n";
    list.reverse();
    std::cout << join(list.to_vector()) << '\n';  // 期望 5 4 3 2 1 0
    check("反转后 front() == 5", list.front() == 5);
    check("反转后 back() == 0", list.back() == 0);

    list.clear();
    check("clear 后 empty()", list.empty());
    check("clear 后 size == 0", list.size() == 0);

    /* ---------- 3. double 类型 ---------- */
    std::cout << "== 3. double 实例化 ==\n";
    LinkedList<double> dlist;
    dlist.push_back(3.14);
    dlist.push_back(2.71);
    dlist.push_front(0.5);
    std::cout << join(dlist.to_vector()) << '\n';  // 期望 0.5 3.14 2.71
    check("double find(2.71) == 2", dlist.find(2.71) == 2);
    dlist.erase(0);
    check("erase 头后 front() == 3.14", dlist.front() == 3.14);

    /* ---------- 4. char 类型 ---------- */
    std::cout << "== 4. char 实例化 ==\n";
    LinkedList<char> clist;
    for (char c = 'a'; c <= 'e'; ++c)
        clist.push_back(c);
    std::cout << join(clist.to_vector()) << '\n';  // 期望 a b c d e
    check("char find('c') == 2", clist.find('c') == 2);
    check("char contains('e')", clist.contains('e'));
    clist.reverse();
    check("char 反转后 front() == 'e'", clist.front() == 'e');

    /* ---------- 5. 深拷贝语义 ---------- */
    std::cout << "== 5. 深拷贝与自赋值 ==\n";
    LinkedList<int> original;
    for (int i = 0; i < 4; ++i)
        original.push_back(i);  // 0 1 2 3
    LinkedList<int> copied = original;  // 拷贝构造
    copied.push_back(100);
    copied.at(0) = -1;
    check("副本修改不影响原表（长度）", original.size() == 4);
    check("副本修改不影响原表（内容）", join(original.to_vector()) == "0 1 2 3");
    check("副本自身已修改", join(copied.to_vector()) == "-1 1 2 3 100");

    LinkedList<int> assigned;
    assigned = original;  // 拷贝赋值
    assigned.erase(0);
    check("赋值副本删除不影响原表", original.front() == 0);

    original = original;  // 自赋值安全
    check("自赋值后内容不变", join(original.to_vector()) == "0 1 2 3");

    /* ---------- 6. 异常语义 ---------- */
    std::cout << "== 6. 越界异常 ==\n";
    bool threw = false;
    try {
        original.at(4);  // size == 4，合法下标最大为 3
    } catch (const std::out_of_range &) {
        threw = true;
    }
    check("at(4) 抛出 out_of_range", threw);

    threw = false;
    try {
        original.insert(5, 1);  // 合法插入范围 [0, 4]
    } catch (const std::out_of_range &) {
        threw = true;
    }
    check("insert(5) 抛出 out_of_range", threw);

    threw = false;
    try {
        LinkedList<int> empty_list;
        empty_list.erase(0);
    } catch (const std::out_of_range &) {
        threw = true;
    }
    check("空表 erase(0) 抛出 out_of_range", threw);

    /* ---------- 7. 尾指针维护（混合操作压力） ---------- */
    std::cout << "== 7. 尾指针一致性 ==\n";
    LinkedList<int> mix;
    mix.push_back(1);
    mix.push_front(0);
    mix.erase(0);   // 删头
    mix.push_back(2);
    mix.erase(1);   // 删尾
    mix.push_back(3);
    check("混合操作后 back() == 3", mix.back() == 3);
    check("混合操作后序列为 1 3", join(mix.to_vector()) == "1 3");

    std::cout << "全部小任务 2 测试完成。\n";
    return 0;
}
