// 同一组回归用例分别编译为基础版/扩展版，期望值独立保存在 std::vector 中。
#ifdef VECTOR_EXTENDED
#include "../task3_extended_vector/vector.hpp"
#else
#include "../task1_vector/vector.hpp"
#endif
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static int checks = 0;
template <typename T>
void expect(const dsaac::Vector<T>& v, const std::vector<T>& expected)
{
    ++checks;
    if (v.to_vector() != expected)
        throw std::runtime_error("Vector 自身引用回归失败");
}

template <typename T>
void test(const std::vector<T>& values)
{
    // 同时覆盖需要扩容/无需扩容、头中尾插入和每个被引用位置。
    for (std::size_t cap : {std::size_t(3), std::size_t(8)}) {
        for (std::size_t source = 0; source < values.size(); ++source) {
            dsaac::Vector<T> v(cap);
            for (const auto& x : values) v.push_back(x);
            auto expected = values;
            expected.push_back(values[source]);
            v.push_back(v[source]);
            expect(v, expected);
            for (std::size_t pos = 0; pos <= values.size(); ++pos) {
                dsaac::Vector<T> w(cap);
                for (const auto& x : values) w.push_back(x);
                auto result = values;
                result.insert(result.begin() + pos, values[source]);
                w.insert(pos, w[source]);
                expect(w, result);
            }
#ifdef VECTOR_EXTENDED
            dsaac::Vector<T> resized(cap);
            for (const auto& x : values) resized.push_back(x);
            auto result = values;
            result.resize(9, values[source]);
            resized.resize(9, resized[source]);
            expect(resized, result);
            for (std::size_t count : {std::size_t(0), std::size_t(2), std::size_t(10)}) {
                dsaac::Vector<T> assigned(cap);
                for (const auto& x : values) assigned.push_back(x);
                assigned.assign(count, assigned[source]);
                expect(assigned, std::vector<T>(count, values[source]));
            }
#endif
        }
    }
}
int main()
{
    try {
        test<int>({1, 2, 3});
        // 长字符串确保测试覆盖实际堆资源，而不只依赖小字符串优化。
        test<std::string>({std::string(100, 'a'), std::string(120, 'b'), std::string(140, 'c')});
        std::cout << "Vector alias regression: " << checks << " PASS\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
