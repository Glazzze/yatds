#ifndef DSAAC_VECTOR_EXT_HPP
#define DSAAC_VECTOR_EXT_HPP

/*
 * Baseline 任务 2 · 小任务 3：可变数组的扩展实现
 *
 * 在小任务 1 的 Vector<T> 基础上，参考 C++ 标准库 <vector>
 * 新增 6 个功能：
 *   pop_back()          删除尾元素（对应 std::vector::pop_back）
 *   resize(n, val)      调整元素个数（对应 std::vector::resize）
 *   swap(other)         交换两个数组的内容（对应 std::vector::swap）
 *   assign(count, val)  填充 count 个值为 val 的元素（对应 std::vector::assign）
 *   find(x)             查找元素下标（std::vector 无直接接口，对照 Java ArrayList.indexOf）
 *   shrink_to_fit()     将容量收缩到与长度一致（对应 std::vector::shrink_to_fit）
 */

#include <cstddef>
#include <vector>

namespace dsaac {

template <typename T>
class Vector {
public:
    explicit Vector(std::size_t capacity = 0);
    Vector(const Vector &other);
    Vector &operator=(const Vector &other);
    ~Vector();

    /* ---- 基本操作（与小任务 1 相同） ---- */

    void push_back(const T &x);
    void insert(std::size_t i, const T &x);
    void erase(std::size_t i);
    T &at(std::size_t i);
    const T &at(std::size_t i) const;
    std::size_t size() const;
    std::size_t capacity() const;
    bool empty() const;
    void clear();

    T &operator[](std::size_t i);
    const T &operator[](std::size_t i) const;
    T &front();
    T &back();
    void reserve(std::size_t new_capacity);
    std::vector<T> to_vector() const;

    /* ---- 扩展功能（参考 std::vector 与常见容器接口） ---- */

    void pop_back();                                  // 删除尾元素，空表抛异常
    void resize(std::size_t n, const T &val = T());   // 改变元素个数（不足补 val，多余截断）
    void swap(Vector &other);                         // 交换内容，O(1)
    void assign(std::size_t count, const T &val);     // 用 count 个 val 重新填充
    int find(const T &x) const;                       // 返回首个 x 的下标，不存在返回 -1
    void shrink_to_fit();                             // 将容量收缩到与长度一致

private:
    T *data_;
    std::size_t size_;
    std::size_t capacity_;
};

} // namespace dsaac

template <typename T>
using Vector = dsaac::Vector<T>;

#endif // DSAAC_VECTOR_EXT_HPP