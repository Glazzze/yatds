/*
 * 小任务 3：命令式测试程序。
 *
 * 按 README 规定的输入输出格式读取 inputs/ 中的样例，
 * 对每个操作驱动 LinkedList<T> 执行并产生输出，再与 outputs/ 中的
 * 标准输出逐行比对，报告每个用例的 PASS / FAIL。
 *
 * 输入格式：
 *   第 1 行：n（初始链表长度）
 *   第 2 行：a1 ... an（初始元素；n = 0 时为空行）
 *   第 3 行：m（操作次数）
 *   接下来 m 行：op args...
 *
 * 操作：PUSH_FRONT / PUSH_BACK / INSERT / ERASE / ACCESS /
 *       FIND / CONTAINS / SIZE / PRINT / CLEAR
 *
 * 输出约定：
 *   产生输出的操作为 ACCESS、FIND、CONTAINS、SIZE、PRINT；
 *   越界（ACCESS / INSERT / ERASE）输出 ERROR；
 *   布尔值输出 true / false；空链表打印输出 EMPTY。
 */
/* 复用小任务 2 的模板链表类（相对路径引用上一级目录的实现） */
#include "../task2_linked_list/linked_list.hpp"

#include <dirent.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/* 执行单个用例：读取输入流，返回程序输出内容（多行拼接） */
static std::string run_case(std::istream &fin)
{
    LinkedList<int> list;
    std::ostringstream out;

    int n;
    fin >> n;
    for (int i = 0; i < n; ++i) {
        int x;
        fin >> x;
        list.push_back(x);
    }

    int m;
    fin >> m;
    for (int i = 0; i < m; ++i) {
        std::string op;
        fin >> op;
        if (op == "PUSH_FRONT") {
            int x;
            fin >> x;
            list.push_front(x);
        } else if (op == "PUSH_BACK") {
            int x;
            fin >> x;
            list.push_back(x);
        } else if (op == "INSERT") {
            long long i;
            int x;
            fin >> i >> x;
            // INSERT 越界输出 ERROR，不中断后续操作
            if (i < 0 || i > static_cast<long long>(list.size()))
                out << "ERROR\n";
            else
                list.insert(static_cast<std::size_t>(i), x);
        } else if (op == "ERASE") {
            long long i;
            fin >> i;
            if (i < 0 || i >= static_cast<long long>(list.size()))
                out << "ERROR\n";
            else
                list.erase(static_cast<std::size_t>(i));
        } else if (op == "ACCESS") {
            long long i;
            fin >> i;
            if (i < 0 || i >= static_cast<long long>(list.size()))
                out << "ERROR\n";
            else
                out << list.at(static_cast<std::size_t>(i)) << '\n';
        } else if (op == "FIND") {
            int x;
            fin >> x;
            out << list.find(x) << '\n';
        } else if (op == "CONTAINS") {
            int x;
            fin >> x;
            out << (list.contains(x) ? "true" : "false") << '\n';
        } else if (op == "SIZE") {
            out << list.size() << '\n';
        } else if (op == "PRINT") {
            std::vector<int> v = list.to_vector();
            if (v.empty()) {
                out << "EMPTY\n";
            } else {
                for (std::size_t j = 0; j < v.size(); ++j) {
                    if (j > 0) out << ' ';
                    out << v[j];
                }
                out << '\n';
            }
        } else if (op == "CLEAR") {
            list.clear();
        }
        // 未知操作静默忽略，保持程序健壮
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
    std::sort(files.begin(), files.end());  // 保证按文件名顺序执行
    return files;
}

int main(int argc, char *argv[])
{
    // 支持两种用法：
    //   ./test                       自动遍历 inputs/ 与 outputs/ 比对
    //   ./test inputs/case01.txt     单用例模式，直接打印程序输出
    if (argc > 1) {
        std::ifstream fin(argv[1]);
        if (!fin.is_open()) {
            std::cerr << "无法打开输入文件: " << argv[1] << '\n';
            return 1;
        }
        std::cout << run_case(fin);
        return 0;
    }

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
        std::string actual = run_case(fin);
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
