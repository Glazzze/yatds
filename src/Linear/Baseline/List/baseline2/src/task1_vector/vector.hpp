#ifndef DSAAC_VECTOR_HPP
#define DSAAC_VECTOR_HPP

/*
 * 可变数组（动态数组）类：既支持像数组一样的随机访问，又支持像链表一样的动态扩容。
 *
 * 底层维护一段连续内存，容量不足时按 2 倍策略扩容；内存用 ::operator new 申请为
 * 原始未初始化存储，元素用 placement new 就地构造、删除时显式析构，构造/析构严格配对
 * （对 std::string 等非平凡类型同样正确）——与 std::vector 的内存管理方式一致。
 * 与 std::vector 相比语义对齐：越界 at/insert/erase 抛 std::out_of_range。
 *
 * 本类为 **header-only**（模板实现全部放在头文件内）：`std::vector` 也是 header-only，
 * 模板实现必须对调用方可见，`push_back` / `operator[]` 等热路径才能被 `-O2` 内联。
 * 若把实现留在 .cpp 里靠显式实例化，跨编译单元的调用无法内联，会引入 2~3 倍的常数级开销
 * （详见 baseline2/README.md 的差异评估与 LTO 实验）。
 */

#include <cstddef>
#include <new>
#include <stdexcept>
#include <vector>

namespace dsaac {

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
    T *data_;               // 连续存储区（原始未初始化内存）
    std::size_t size_;      // 元素个数
    std::size_t capacity_;  // 当前容量
};

/* ---------- 实现（header-only） ---------- */

/* 构造：预留 capacity 个元素的原始存储（不构造任何 T 对象） */
template <typename T>
Vector<T>::Vector(std::size_t capacity)
{
    data_ = capacity > 0 ? static_cast<T *>(::operator new(capacity * sizeof(T))) : nullptr;
    size_ = 0;
    capacity_ = capacity;
}

/* 拷贝构造：申请同容量原始内存，逐个 placement new 复制有效元素 */
template <typename T>
Vector<T>::Vector(const Vector &other)
{
    capacity_ = other.capacity_;
    size_ = other.size_;
    data_ = capacity_ > 0 ? static_cast<T *>(::operator new(capacity_ * sizeof(T))) : nullptr;
    for (std::size_t i = 0; i < size_; i++)
        new (data_ + i) T(other.data_[i]);
}

/* 拷贝赋值：先释放自身，再深拷贝（自赋值安全） */
template <typename T>
Vector<T> &Vector<T>::operator=(const Vector &other)
{
    if (this != &other) {
        for (std::size_t i = 0; i < size_; i++)  // 销毁自身有效元素
            data_[i].~T();
        ::operator delete(data_);               // 释放原始内存
        capacity_ = other.capacity_;
        size_ = other.size_;
        data_ = capacity_ > 0 ? static_cast<T *>(::operator new(capacity_ * sizeof(T))) : nullptr;
        for (std::size_t i = 0; i < size_; i++)
            new (data_ + i) T(other.data_[i]);
    }
    return *this;
}

/* 析构：销毁全部有效元素，再释放原始内存 */
template <typename T>
Vector<T>::~Vector()
{
    for (std::size_t i = 0; i < size_; i++)
        data_[i].~T();
    ::operator delete(data_);
}

/* 预留容量：只增不减；扩容时申请新原始内存并逐个 placement new 搬移既有元素 */
template <typename T>
void Vector<T>::reserve(std::size_t new_capacity)
{
    if (new_capacity <= capacity_)
        return;
    T *new_data = static_cast<T *>(::operator new(new_capacity * sizeof(T)));
    for (std::size_t i = 0; i < size_; i++)
        new (new_data + i) T(data_[i]);         // 就地构造搬移
    for (std::size_t i = 0; i < size_; i++)
        data_[i].~T();                          // 析构旧元素
    ::operator delete(data_);
    data_ = new_data;
    capacity_ = new_capacity;
}

/* 尾部追加：容量不足时按 2 倍扩容（首次扩容从 0 到 1） */
template <typename T>
void Vector<T>::push_back(const T &x)
{
    if (size_ == capacity_)
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    new (data_ + size_) T(x);                   // 就地在尾部构造
    size_++;
}

/* 在下标 i 处插入：合法范围 [0, size]（i == size 等价尾插） */
template <typename T>
void Vector<T>::insert(std::size_t i, const T &x)
{
    if (i > size_)
        throw std::out_of_range("insert index out of range");
    if (size_ == capacity_)
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    if (i == size_) {
        new (data_ + size_) T(x);               // 追加：直接构造
    } else {
        /* 尾部先构造末元素副本，再把 [i, size) 区间整体后移一格，最后赋 x */
        new (data_ + size_) T(data_[size_ - 1]);
        for (std::size_t k = size_ - 1; k > i; k--)
            data_[k] = data_[k - 1];
        data_[i] = x;
    }
    size_++;
}

/* 删除下标 i 处元素：将 (i, size) 区间元素前移一格，并析构末尾元素 */
template <typename T>
void Vector<T>::erase(std::size_t i)
{
    if (i >= size_)
        throw std::out_of_range("erase index out of range");
    for (std::size_t k = i; k + 1 < size_; k++)
        data_[k] = data_[k + 1];
    data_[size_ - 1].~T();                      // 析构被移出的末尾元素
    size_--;
}

/* 访问下标 i：越界抛异常，与 std::vector::at 行为一致 */
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

/* 清空元素：析构全部有效元素，保留已分配容量（不缩容，供后续复用） */
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

/* 访问首元素 */
template <typename T>
T &Vector<T>::front()
{
    if (empty())
        throw std::out_of_range("front on empty vector");
    return data_[0];
}

/* 访问尾元素 */
template <typename T>
T &Vector<T>::back()
{
    if (empty())
        throw std::out_of_range("back on empty vector");
    return data_[size_ - 1];
}

/* 导出元素序列 */
template <typename T>
std::vector<T> Vector<T>::to_vector() const
{
    std::vector<T> values;
    values.reserve(size_);
    for (std::size_t i = 0; i < size_; i++)
        values.push_back(data_[i]);
    return values;
}

} // namespace dsaac

template <typename T>
using Vector = dsaac::Vector<T>;

#endif // DSAAC_VECTOR_HPP