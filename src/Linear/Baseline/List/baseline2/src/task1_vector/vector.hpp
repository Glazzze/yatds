#ifndef DSAAC_VECTOR_HPP
#define DSAAC_VECTOR_HPP

#include <cstddef>
#include <vector>

namespace dsaac {

/*
 * 可变数组（动态数组）类：既支持像数组一样的随机访问，又支持像链表一样的动态扩容。
 *
 * 底层维护一段连续内存，容量不足时按 2 倍策略扩容；元素内存由构造函数、
 * 拷贝构造、拷贝赋值与析构函数统一管理（RAII），调用方无须手动释放。
 * 与 std::vector 相比语义对齐：越界 at/insert/erase 抛 std::out_of_range。
 */
template <typename T>
class Vector {
public:
    /* capacity 为初始容量（预留 capacity 个元素的连续空间） */
    explicit Vector(std::size_t capacity = 0);
    Vector(const Vector &other);             // 深拷贝
    Vector &operator=(const Vector &other);  // 深拷贝赋值
    ~Vector();                                // 释放连续内存

    /* ---------- 基本操作 ---------- */

    void push_back(const T &x);              // 尾部追加（容量不足自动扩容）
    void insert(std::size_t i, const T &x);  // 在下标 i 处插入，越界抛异常
    void erase(std::size_t i);               // 删除下标 i 处元素，越界抛异常
    T &at(std::size_t i);                    // 访问下标 i，越界抛 out_of_range
    const T &at(std::size_t i) const;
    std::size_t size() const;                // 元素个数
    std::size_t capacity() const;            // 当前容量
    bool empty() const;                      // 是否为空
    void clear();                            // 清空元素（保留容量，不缩容）

    /* ---------- 扩展操作 ---------- */

    T &operator[](std::size_t i);            // 无边界检查访问（与 std::vector 一致）
    const T &operator[](std::size_t i) const;
    T &front();                              // 首元素（空表抛异常）
    T &back();                               // 尾元素（空表抛异常）
    void reserve(std::size_t new_capacity);  // 预留容量（只增不减）
    std::vector<T> to_vector() const;        // 导出元素序列，便于打印与比对

private:
    T *data_;               // 连续存储区（堆内存）
    std::size_t size_;      // 元素个数
    std::size_t capacity_;  // 当前容量
};

} // namespace dsaac

template <typename T>
using Vector = dsaac::Vector<T>;

#endif // DSAAC_VECTOR_HPP