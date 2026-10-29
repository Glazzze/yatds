#include "core/linked_stack.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
struct CopyFailure {
    CopyFailure() = default;
    CopyFailure(const CopyFailure &) { throw std::runtime_error("copy failed"); }
    CopyFailure(CopyFailure &&) = default;
};

struct Counted {
    static int alive;
    Counted() { ++alive; }
    Counted(Counted &&) { ++alive; }
    ~Counted() { --alive; }
};
int Counted::alive = 0;
} // namespace

int main()
{
    zuma::LinkedStack<int> stack;
    assert(stack.empty());
    int rejected = 0;
    try { stack.top(); } catch (const std::out_of_range &) { ++rejected; }
    const auto &readOnly = stack;
    try { readOnly.top(); } catch (const std::out_of_range &) { ++rejected; }
    try { stack.pop(); } catch (const std::out_of_range &) { ++rejected; }
    assert(rejected == 3 && stack.empty());

    const int first = 7;
    stack.push(first); // 复制入栈
    stack.push(9);
    assert(readOnly.top() == 9);
    stack.pop();
    assert(stack.top() == 7);
    stack.pop();
    assert(stack.empty());

    zuma::LinkedStack<std::unique_ptr<int>> owned;
    owned.push(std::make_unique<int>(42)); // 支持仅可移动的值
    assert(*owned.top() == 42);
    owned.pop();

    zuma::LinkedStack<CopyFailure> throwing;
    throwing.push(CopyFailure{});
    const auto *previous = &throwing.top();
    CopyFailure value;
    bool failed = false;
    try { throwing.push(value); } catch (const std::runtime_error &) { failed = true; }
    assert(failed && &throwing.top() == previous); // 构造失败保留原栈顶
    throwing.pop();
    assert(throwing.empty());

    {
        zuma::LinkedStack<Counted> deep;
        for (int i = 0; i < 100000; ++i)
            deep.push(Counted{});
        assert(Counted::alive == 100000);
    }
    assert(Counted::alive == 0); // 深栈迭代析构，全部对象释放
    std::cout << "ALL LINKED STACK TESTS PASSED\n";
}
