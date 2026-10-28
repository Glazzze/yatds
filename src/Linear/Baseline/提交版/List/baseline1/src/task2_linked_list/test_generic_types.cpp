#include "linked_list.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

// 无默认构造函数，验证模板不依赖 int/double/char 的显式实例化。
struct Record {
    int id;
    explicit Record(int value) : id(value) {}
    bool operator==(const Record& other) const { return id == other.id; }
};
static int checks = 0;
static void expect(bool ok)
{
    ++checks;
    if (!ok) throw std::runtime_error("泛型链表回归失败");
}
int main()
{
    try {
        LinkedList<std::string> words;
        words.push_back("alpha"); words.push_front("beta");
        expect(words.at(0) == "beta");
        expect(words.find("alpha") == 1);
        auto copied = words;
        copied.at(0) = "changed";
        expect(words.at(0) == "beta");
        LinkedList<Record> records;
        records.push_back(Record(7)); records.push_front(Record(3));
        records.insert(1, Record(5));
        expect(records.find(Record(5)) == 1);
        records.reverse();
        expect(records.front().id == 7 && records.back().id == 3);
        LinkedList<Record> assigned;
        assigned = records;
        assigned.erase(0);
        expect(records.size() == 3 && assigned.size() == 2);
        assigned.clear();
        expect(assigned.empty());
        std::cout << "Generic LinkedList regression: " << checks << " PASS\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; return 1;
    }
}
