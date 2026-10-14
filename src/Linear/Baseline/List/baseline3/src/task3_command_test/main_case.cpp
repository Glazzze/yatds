/*
 * Baseline 任务 3 · 命令式测试程序（编译为 test）
 *
 * 按课程 README 规定的输入输出格式读取 inputs/ 中的样例，对每个场景求解
 * 「幽灵交叉路口」综合题（探环解环 → 找交汇点 → 反转公共部分），与 outputs/
 * 中的标准输出逐用例比对，报告每个用例的 PASS / FAIL。
 *
 * 输入格式：
 *   第 1 行 T（场景数量）；随后 T 个场景，每个场景第 1 行为场景类型：
 *     CYCLE           n entry；下一行 n 个结点值（entry 为环入口下标，-1 表示无环）
 *     NO_INTERSECTION nA 及 nA 个值；再 nB 及 nB 个值
 *     INTERSECTION    nA 及 ListA 独有前缀；nB 及 ListB 独有前缀；k 及共享公共尾部
 *
 * 输出格式：
 *   每个场景先输出 CASE t（t 从 1 开始）；
 *   CYCLE          输出 CYCLE_ENTRY <入口值|NONE>，随后输出解环后的链表
 *   NO_INTERSECTION 输出 INTERSECTION NONE
 *   INTERSECTION   输出 INTERSECTION <交汇值>，随后输出公共部分反转后的 LIST_A 与 LIST_B
 *   链表打印：元素空格分隔，空链表输出 EMPTY
 *
 * 两种用法：
 *   ./test                  自动遍历 inputs/ 与 outputs/ 比对
 *   ./test inputs/case01.txt 单用例模式，直接打印程序输出，便于调试
 */

#include "../task1_algorithms/ghost_intersection.hpp"
#include "../common/basic_list/list.hpp"

#include <dirent.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/* 打印一条链表到输出流：元素空格分隔，空链表输出 EMPTY */
static void printList(std::ostringstream &out, const std::string &label, ListNode *head)
{
    std::vector<int> values = dsaac::to_vector(head);
    out << label;
    if (values.empty()) {
        out << "EMPTY\n";
        return;
    }
    for (std::size_t i = 0; i < values.size(); i++) {
        if (i > 0)
            out << ' ';
        out << values[i];
    }
    out << '\n';
}

/* 读取 n 个整数（operator>> 自动跳过空白与换行，对 n=0 与空行均健壮） */
static std::vector<int> readValues(std::istream &fin, int n)
{
    std::vector<int> values;
    values.reserve(n > 0 ? static_cast<std::size_t>(n) : 0);
    for (int i = 0; i < n; i++) {
        int x;
        if (!(fin >> x))
            break;
        values.push_back(x);
    }
    return values;
}

/* 执行单个用例（整套输入流含 T 个场景），返回全部输出内容 */
static std::string runCase(std::istream &fin)
{
    std::ostringstream out;

    int T = 0;
    if (!(fin >> T))
        return out.str();

    for (int t = 1; t <= T; t++) {
        std::string type;
        fin >> type;
        out << "CASE " << t << '\n';

        if (type == "CYCLE") {
            int n = 0, entry = -1;
            fin >> n >> entry;
            std::vector<int> values = readValues(fin, n);

            Scenario sc; // RAII：析构时自动解环并按正确规则释放
            sc.headA = build_cycle(values, entry);
            /* entry 越界（含 n=0 而 entry!=-1）时按无环处理，避免空指针解引用 */
            ListNode *cycleEntry = GhostIntersection::detectAndRemoveCycle(sc.headA);
            if (cycleEntry != nullptr)
                out << "CYCLE_ENTRY " << cycleEntry->val << '\n';
            else
                out << "CYCLE_ENTRY NONE\n";
            printList(out, "LIST ", sc.headA);
        } else if (type == "NO_INTERSECTION") {
            int nA = 0;
            fin >> nA;
            std::vector<int> a = readValues(fin, nA);
            int nB = 0;
            fin >> nB;
            std::vector<int> b = readValues(fin, nB);

            Scenario sc;
            sc.headA = dsaac::build_list(a);
            sc.headB = dsaac::build_list(b);
            ListNode *inter = GhostIntersection::getIntersectionNode(sc.headA, sc.headB);
            if (inter != nullptr)
                out << "INTERSECTION " << inter->val << '\n';
            else
                out << "INTERSECTION NONE\n";
        } else if (type == "INTERSECTION") {
            int nA = 0;
            fin >> nA;
            std::vector<int> pa = readValues(fin, nA);
            int nB = 0;
            fin >> nB;
            std::vector<int> pb = readValues(fin, nB);
            int k = 0;
            fin >> k;
            std::vector<int> shared = readValues(fin, k);

            Scenario sc;
            sc.boundary = build_shared(pa, pb, shared, sc.headA, sc.headB);
            ListNode *inter = GhostIntersection::getIntersectionNode(sc.headA, sc.headB);
            if (inter != nullptr) {
                out << "INTERSECTION " << inter->val << '\n';
                sc.reverseShared(); // 仅反转公共部分（RAII 会同步更新 boundary）
                printList(out, "LIST_A ", sc.headA);
                printList(out, "LIST_B ", sc.headB);
            } else {
                out << "INTERSECTION NONE\n";
            }
        }
        // 未知场景类型静默忽略，保持程序健壮
    }
    return out.str();
}

/* 读取整个文件内容（标准输出文件）为字符串 */
static std::string read_file(const std::string &path)
{
    std::ifstream f(path);
    if (!f.is_open())
        return std::string();
    std::ostringstream oss;
    oss << f.rdbuf();
    return oss.str();
}

/* 枚举目录下的全部 .txt 文件名（不含路径） */
static std::vector<std::string> list_case_files(const std::string &dir)
{
    std::vector<std::string> files;
    DIR *dp = opendir(dir.c_str());
    if (dp == nullptr)
        return files;
    struct dirent *entry;
    while ((entry = readdir(dp)) != nullptr) {
        std::string name = entry->d_name;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt")
            files.push_back(name);
    }
    closedir(dp);
    std::sort(files.begin(), files.end()); // 保证按文件名顺序执行
    return files;
}

int main(int argc, char *argv[])
{
    // 单用例模式：直接打印输出，便于调试
    if (argc > 1) {
        std::ifstream fin(argv[1]);
        if (!fin.is_open()) {
            std::cerr << "无法打开输入文件: " << argv[1] << '\n';
            return 1;
        }
        std::cout << runCase(fin);
        return 0;
    }

    // 自动遍历模式：逐一比对 inputs/ 与 outputs/
    const std::string input_dir = "inputs";
    const std::string output_dir = "outputs";
    std::vector<std::string> cases = list_case_files(input_dir);
    if (cases.empty()) {
        std::cerr << "inputs/ 目录中没有找到 .txt 样例\n";
        return 1;
    }

    int passed = 0;
    for (const std::string &name : cases) {
        std::ifstream fin(input_dir + "/" + name);
        if (!fin.is_open()) {
            std::cout << name << ": FAIL (无法打开输入文件)\n";
            continue;
        }
        std::string expected = read_file(output_dir + "/" + name);
        std::string actual = runCase(fin);
        if (actual == expected) {
            std::cout << name << ": PASS\n";
            passed++;
        } else {
            std::cout << name << ": FAIL\n";
            std::cout << "--- 期望 ---\n" << expected;
            std::cout << "--- 实际 ---\n" << actual;
        }
    }
    std::cout << "共 " << cases.size() << " 个用例，通过 " << passed << " 个\n";
    return passed == static_cast<int>(cases.size()) ? 0 : 1;
}