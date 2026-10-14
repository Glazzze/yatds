#include "vector.hpp"

#include <stdexcept>
#include <utility>

/* START: vector implementation */

/* 构造：预留 capacity 个元素的连续空间 */
template <typename T>
dsaac::Vector<T>::Vector(std::size_t capacity)
{
    data_ = capacity > 0 ? new T[capacity] : nullptr;
    size_ = 0;
    capacity_ = capacity;
}

/* 拷贝构造：按 other 的容量与元素逐个深拷贝，两个对象互不影响 */
template <typename T>
dsaac::Vector<T>::Vector(const Vector &other)
{
    capacity_ = other.capacity_;
    size_ = other.size_;
    data_ = capacity_ > 0 ? new T[capacity_] : nullptr;
    for (std::size_t i = 0; i < size_; i++)
        data_[i] = other.data_[i];
}

/* 拷贝赋值：先释放自身内存，再深拷贝（自赋值安全） */
template <typename T>
dsaac::Vector<T> &dsaac::Vector<T>::operator=(const Vector &other)
{
    if (this != &other) {
        delete[] data_;
        capacity_ = other.capacity_;
        size_ = other.size_;
        data_ = capacity_ > 0 ? new T[capacity_] : nullptr;
        for (std::size_t i = 0; i < size_; i++)
            data_[i] = other.data_[i];
    }
    return *this;
}

/* 析构：释放连续内存 */
template <typename T>
dsaac::Vector<T>::~Vector()
{
    delete[] data_;
}

/* 预留容量：只增不减；扩容时申请新内存并搬移已有元素 */
template <typename T>
void dsaac::Vector<T>::reserve(std::size_t new_capacity)
{
    if (new_capacity <= capacity_)
        return;
    T *new_data = new T[new_capacity];
    for (std::size_t i = 0; i < size_; i++)
        new_data[i] = data_[i];
    delete[] data_;
    data_ = new_data;
    capacity_ = new_capacity;
}

/* 尾部追加：容量不足时按 2 倍扩容（首次扩容从 0 到 1） */
template <typename T>
void dsaac::Vector<T>::push_back(const T &x)
{
    if (size_ == capacity_)
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    data_[size_] = x;
    size_++;
}

/* 在下标 i 处插入：合法范围 [0, size]（i == size 等价尾插） */
template <typename T>
void dsaac::Vector<T>::insert(std::size_t i, const T &x)
{
    if (i > size_)
        throw std::out_of_range("insert index out of range");
    if (size_ == capacity_)
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    // 将 [i, size) 区间元素整体后移一格，为 x 让出位置
    for (std::size_t k = size_; k > i; k--)
        data_[k] = data_[k - 1];
    data_[i] = x;
    size_++;
}

/* 删除下标 i 处元素：将 (i, size) 区间元素整体前移一格 */
template <typename T>
void dsaac::Vector<T>::erase(std::size_t i)
{
    if (i >= size_)
        throw std::out_of_range("erase index out of range");
    for (std::size_t k = i; k + 1 < size_; k++)
        data_[k] = data_[k + 1];
    size_--;
}

/* 访问下标 i：越界抛异常，与 std::vector::at 行为一致 */
template <typename T>
T &dsaac::Vector<T>::at(std::size_t i)
{
    if (i >= size_)
        throw std::out_of_range("at index out of range");
    return data_[i];
}

template <typename T>
const T &dsaac::Vector<T>::at(std::size_t i) const
{
    if (i >= size_)
        throw std::out_of_range("at index out of range");
    return data_[i];
}

template <typename T>
std::size_t dsaac::Vector<T>::size() const
{
    return size_;
}

template <typename T>
std::size_t dsaac::Vector<T>::capacity() const
{
    return capacity_;
}

template <typename T>
bool dsaac::Vector<T>::empty() const
{
    return size_ == 0;
}

/* 清空元素：仅将长度归零，保留已分配的容量（不缩容，供后续复用） */
template <typename T>
void dsaac::Vector<T>::clear()
{
    size_ = 0;
}

template <typename T>
T &dsaac::Vector<T>::operator[](std::size_t i)
{
    return data_[i];
}

template <typename T>
const T &dsaac::Vector<T>::operator[](std::size_t i) const
{
    return data_[i];
}

/* 访问首元素 */
template <typename T>
T &dsaac::Vector<T>::front()
{
    if (empty())
        throw std::out_of_range("front on empty vector");
    return data_[0];
}

/* 访问尾元素 */
template <typename T>
T &dsaac::Vector<T>::back()
{
    if (empty())
        throw std::out_of_range("back on empty vector");
    return data_[size_ - 1];
}

/* 导出元素序列 */
template <typename T>
std::vector<T> dsaac::Vector<T>::to_vector() const
{
    std::vector<T> values;
    values.reserve(size_);
    for (std::size_t i = 0; i < size_; i++)
        values.push_back(data_[i]);
    return values;
}

/* 显式实例化常用类型，保证 vector.cpp 与测试程序分开编译时链接通过 */
template class dsaac::Vector<int>;
template class dsaac::Vector<double>;
template class dsaac::Vector<char>;

/* END */