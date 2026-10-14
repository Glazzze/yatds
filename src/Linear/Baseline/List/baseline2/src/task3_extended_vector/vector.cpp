#include "vector.hpp"

#include <stdexcept>
#include <utility>

/* START: vector implementation */

template <typename T>
dsaac::Vector<T>::Vector(std::size_t capacity)
{
    data_ = capacity > 0 ? new T[capacity] : nullptr;
    size_ = 0;
    capacity_ = capacity;
}

template <typename T>
dsaac::Vector<T>::Vector(const Vector &other)
{
    capacity_ = other.capacity_;
    size_ = other.size_;
    data_ = capacity_ > 0 ? new T[capacity_] : nullptr;
    for (std::size_t i = 0; i < size_; i++)
        data_[i] = other.data_[i];
}

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

template <typename T>
dsaac::Vector<T>::~Vector()
{
    delete[] data_;
}

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

template <typename T>
void dsaac::Vector<T>::push_back(const T &x)
{
    if (size_ == capacity_)
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    data_[size_] = x;
    size_++;
}

template <typename T>
void dsaac::Vector<T>::insert(std::size_t i, const T &x)
{
    if (i > size_)
        throw std::out_of_range("insert index out of range");
    if (size_ == capacity_)
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    for (std::size_t k = size_; k > i; k--)
        data_[k] = data_[k - 1];
    data_[i] = x;
    size_++;
}

template <typename T>
void dsaac::Vector<T>::erase(std::size_t i)
{
    if (i >= size_)
        throw std::out_of_range("erase index out of range");
    for (std::size_t k = i; k + 1 < size_; k++)
        data_[k] = data_[k + 1];
    size_--;
}

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

template <typename T>
T &dsaac::Vector<T>::front()
{
    if (empty())
        throw std::out_of_range("front on empty vector");
    return data_[0];
}

template <typename T>
T &dsaac::Vector<T>::back()
{
    if (empty())
        throw std::out_of_range("back on empty vector");
    return data_[size_ - 1];
}

template <typename T>
std::vector<T> dsaac::Vector<T>::to_vector() const
{
    std::vector<T> values;
    values.reserve(size_);
    for (std::size_t i = 0; i < size_; i++)
        values.push_back(data_[i]);
    return values;
}

/* ---- 扩展功能 ---- */

/* 删除尾元素：空表时报错 */
template <typename T>
void dsaac::Vector<T>::pop_back()
{
    if (empty())
        throw std::out_of_range("pop_back on empty vector");
    size_--;
}

/* 改变元素个数：n < size 时截断；n > size 时用 val 补足（必要时扩容） */
template <typename T>
void dsaac::Vector<T>::resize(std::size_t n, const T &val)
{
    if (n < size_) {
        size_ = n;
    } else if (n > size_) {
        if (n > capacity_)
            reserve(n);
        for (std::size_t i = size_; i < n; i++)
            data_[i] = val;
        size_ = n;
    }
}

/* 交换内容：只交换底层指针与长度，O(1) */
template <typename T>
void dsaac::Vector<T>::swap(Vector &other)
{
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
}

/* 用 count 个 val 重新填充（先清空再逐个写入） */
template <typename T>
void dsaac::Vector<T>::assign(std::size_t count, const T &val)
{
    clear();
    if (count > capacity_)
        reserve(count);
    for (std::size_t i = 0; i < count; i++)
        data_[i] = val;
    size_ = count;
}

/* 查找首个值为 x 的下标，不存在返回 -1（线性查找 O(n)） */
template <typename T>
int dsaac::Vector<T>::find(const T &x) const
{
    for (std::size_t i = 0; i < size_; i++) {
        if (data_[i] == x)
            return static_cast<int>(i);
    }
    return -1;
}

/* 将容量收缩到恰好容纳当前元素，释放多余内存 */
template <typename T>
void dsaac::Vector<T>::shrink_to_fit()
{
    if (size_ == capacity_)
        return;
    T *new_data = size_ > 0 ? new T[size_] : nullptr;
    for (std::size_t i = 0; i < size_; i++)
        new_data[i] = data_[i];
    delete[] data_;
    data_ = new_data;
    capacity_ = size_;
}

/* 显式实例化常用类型 */
template class dsaac::Vector<int>;
template class dsaac::Vector<double>;
template class dsaac::Vector<char>;

/* END */