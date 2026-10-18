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
 *
 * 内存管理与小任务 1 一致：::operator new 原始内存 + placement new 就地构造 / 显式析构。
 * 本类为 **header-only**（模板实现全部放在头文件内），使 push_back / operator[] 等热路径
 * 可被 -O2 跨编译单元内联（同 std::vector），避免常数级开销。
 */

#include <cstddef>
#include <new>
#include <stdexcept>
#include <utility>
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

/* ---------- 实现（header-only） ---------- */

template <typename T>
Vector<T>::Vector(std::size_t capacity)
{
    data_ = capacity > 0 ? static_cast<T *>(::operator new(capacity * sizeof(T))) : nullptr;
    size_ = 0;
    capacity_ = capacity;
}

template <typename T>
Vector<T>::Vector(const Vector &other)
{
    capacity_ = other.capacity_;
    size_ = other.size_;
    data_ = capacity_ > 0 ? static_cast<T *>(::operator new(capacity_ * sizeof(T))) : nullptr;
    for (std::size_t i = 0; i < size_; i++)
        new (data_ + i) T(other.data_[i]);
}

template <typename T>
Vector<T> &Vector<T>::operator=(const Vector &other)
{
    if (this != &other) {
        for (std::size_t i = 0; i < size_; i++)
            data_[i].~T();
        ::operator delete(data_);
        capacity_ = other.capacity_;
        size_ = other.size_;
        data_ = capacity_ > 0 ? static_cast<T *>(::operator new(capacity_ * sizeof(T))) : nullptr;
        for (std::size_t i = 0; i < size_; i++)
            new (data_ + i) T(other.data_[i]);
    }
    return *this;
}

template <typename T>
Vector<T>::~Vector()
{
    for (std::size_t i = 0; i < size_; i++)
        data_[i].~T();
    ::operator delete(data_);
}

template <typename T>
void Vector<T>::reserve(std::size_t new_capacity)
{
    if (new_capacity <= capacity_)
        return;
    T *new_data = static_cast<T *>(::operator new(new_capacity * sizeof(T)));
    for (std::size_t i = 0; i < size_; i++)
        new (new_data + i) T(data_[i]);
    for (std::size_t i = 0; i < size_; i++)
        data_[i].~T();
    ::operator delete(data_);
    data_ = new_data;
    capacity_ = new_capacity;
}

template <typename T>
void Vector<T>::push_back(const T &x)
{
    if (size_ == capacity_)
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    new (data_ + size_) T(x);
    size_++;
}

template <typename T>
void Vector<T>::insert(std::size_t i, const T &x)
{
    if (i > size_)
        throw std::out_of_range("insert index out of range");
    if (size_ == capacity_)
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    if (i == size_) {
        new (data_ + size_) T(x);
    } else {
        new (data_ + size_) T(data_[size_ - 1]);
        for (std::size_t k = size_ - 1; k > i; k--)
            data_[k] = data_[k - 1];
        data_[i] = x;
    }
    size_++;
}

template <typename T>
void Vector<T>::erase(std::size_t i)
{
    if (i >= size_)
        throw std::out_of_range("erase index out of range");
    for (std::size_t k = i; k + 1 < size_; k++)
        data_[k] = data_[k + 1];
    data_[size_ - 1].~T();
    size_--;
}

template <typename T>
T &Vector<T>::at(std::size_t i)
{
    if (i >= size_)
        throw std::out_of_range("at index out of range");
    return data_[i];
}

template <typename T>
const T &Vector<T>::at(std::size_t i) const
{
    if (i >= size_)
        throw std::out_of_range("at index out of range");
    return data_[i];
}

template <typename T>
std::size_t Vector<T>::size() const
{
    return size_;
}

template <typename T>
std::size_t Vector<T>::capacity() const
{
    return capacity_;
}

template <typename T>
bool Vector<T>::empty() const
{
    return size_ == 0;
}

template <typename T>
void Vector<T>::clear()
{
    for (std::size_t i = 0; i < size_; i++)
        data_[i].~T();
    size_ = 0;
}

template <typename T>
T &Vector<T>::operator[](std::size_t i)
{
    return data_[i];
}

template <typename T>
const T &Vector<T>::operator[](std::size_t i) const
{
    return data_[i];
}

template <typename T>
T &Vector<T>::front()
{
    if (empty())
        throw std::out_of_range("front on empty vector");
    return data_[0];
}

template <typename T>
T &Vector<T>::back()
{
    if (empty())
        throw std::out_of_range("back on empty vector");
    return data_[size_ - 1];
}

template <typename T>
std::vector<T> Vector<T>::to_vector() const
{
    std::vector<T> values;
    values.reserve(size_);
    for (std::size_t i = 0; i < size_; i++)
        values.push_back(data_[i]);
    return values;
}

/* ---- 扩展功能 ---- */

/* 删除尾元素：空表时报错；析构被移除的元素（对非平凡类型立即回收资源） */
template <typename T>
void Vector<T>::pop_back()
{
    if (empty())
        throw std::out_of_range("pop_back on empty vector");
    data_[size_ - 1].~T();
    size_--;
}

/* 改变元素个数：n < size 时析构多余元素；n > size 时用 val 就地补足（必要时扩容） */
template <typename T>
void Vector<T>::resize(std::size_t n, const T &val)
{
    if (n < size_) {
        for (std::size_t i = n; i < size_; i++)
            data_[i].~T();
        size_ = n;
    } else if (n > size_) {
        if (n > capacity_)
            reserve(n);
        for (std::size_t i = size_; i < n; i++)
            new (data_ + i) T(val);
        size_ = n;
    }
}

/* 交换内容：只交换底层指针与长度，O(1) */
template <typename T>
void Vector<T>::swap(Vector &other)
{
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
}

/* 用 count 个 val 重新填充：先析构全部旧元素，再就地构造 count 个副本 */
template <typename T>
void Vector<T>::assign(std::size_t count, const T &val)
{
    for (std::size_t i = 0; i < size_; i++)
        data_[i].~T();
    size_ = 0;
    if (count > capacity_)
        reserve(count);
    for (std::size_t i = 0; i < count; i++)
        new (data_ + i) T(val);
    size_ = count;
}

/* 查找首个值为 x 的下标，不存在返回 -1（线性查找 O(n)） */
template <typename T>
int Vector<T>::find(const T &x) const
{
    for (std::size_t i = 0; i < size_; i++) {
        if (data_[i] == x)
            return static_cast<int>(i);
    }
    return -1;
}

/* 将容量收缩到恰好容纳当前元素，释放多余内存 */
template <typename T>
void Vector<T>::shrink_to_fit()
{
    if (size_ == capacity_)
        return;
    T *new_data = size_ > 0 ? static_cast<T *>(::operator new(size_ * sizeof(T))) : nullptr;
    for (std::size_t i = 0; i < size_; i++)
        new (new_data + i) T(data_[i]);
    for (std::size_t i = 0; i < size_; i++)
        data_[i].~T();
    ::operator delete(data_);
    data_ = new_data;
    capacity_ = size_;
}

} // namespace dsaac

template <typename T>
using Vector = dsaac::Vector<T>;

#endif // DSAAC_VECTOR_EXT_HPP