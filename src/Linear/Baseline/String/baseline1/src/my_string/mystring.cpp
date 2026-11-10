#include "mystring.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

/* START: mystring implementation */

namespace {

/* 求 C 风格字符串长度（不依赖 std::strlen 的调用约定，保持实现直观） */
std::size_t cstrLength(const char *str)
{
    std::size_t n = 0;
    if (str == nullptr)
        return 0;
    while (str[n] != '\0')
        ++n;
    return n;
}

} // namespace

dsaac::MyString::MyString()
    : data_(new char[1]), len_(0), cap_(1), growthFactor_(2.0), reallocCount_(0)
{
    data_[0] = '\0';
}

dsaac::MyString::MyString(const char *str)
    : len_(cstrLength(str)), cap_(len_ + 1), growthFactor_(2.0), reallocCount_(0)
{
    data_ = new char[cap_];
    for (std::size_t i = 0; i < len_; ++i)
        data_[i] = str[i];
    data_[len_] = '\0';
}

dsaac::MyString::MyString(const char *str, size_type count)
    : len_(count), cap_(count + 1), growthFactor_(2.0), reallocCount_(0)
{
    data_ = new char[cap_];
    for (std::size_t i = 0; i < count; ++i)
        data_[i] = str[i];
    data_[count] = '\0';
}

dsaac::MyString::~MyString()
{
    delete[] data_;
}

dsaac::MyString::MyString(const MyString &other)
    : len_(other.len_), cap_(other.cap_), growthFactor_(other.growthFactor_),
      reallocCount_(other.reallocCount_)
{
    data_ = new char[cap_];
    std::memcpy(data_, other.data_, len_ + 1);
}

dsaac::MyString &dsaac::MyString::operator=(const MyString &other)
{
    if (this != &other) {
        /* 先申请新内存再释放旧内存，避免 other 与本对象共享存储时自伤 */
        char *newData = new char[other.len_ + 1];
        std::memcpy(newData, other.data_, other.len_ + 1);
        delete[] data_;
        data_ = newData;
        len_ = other.len_;
        cap_ = other.len_ + 1;
    }
    return *this;
}

dsaac::MyString::MyString(MyString &&other) noexcept
    : data_(other.data_), len_(other.len_), cap_(other.cap_),
      growthFactor_(other.growthFactor_), reallocCount_(other.reallocCount_)
{
    other.data_ = nullptr;
    other.len_ = 0;
    other.cap_ = 0;
}

dsaac::MyString &dsaac::MyString::operator=(MyString &&other) noexcept
{
    if (this != &other) {
        delete[] data_;
        data_ = other.data_;
        len_ = other.len_;
        cap_ = other.cap_;
        growthFactor_ = other.growthFactor_;
        reallocCount_ = other.reallocCount_;
        other.data_ = nullptr;
        other.len_ = 0;
        other.cap_ = 0;
    }
    return *this;
}

std::size_t dsaac::MyString::length() const
{
    return len_;
}

std::size_t dsaac::MyString::size() const
{
    return len_;
}

bool dsaac::MyString::empty() const
{
    return len_ == 0;
}

std::size_t dsaac::MyString::capacity() const
{
    return cap_;
}

const char *dsaac::MyString::c_str() const
{
    return data_;
}

void dsaac::MyString::ensureCapacity(size_type need)
{
    if (cap_ >= need)
        return;

    size_type newCap;
    if (growthFactor_ <= 0.0) {
        /* +1 语义：每次只扩到刚好够用，用于演示 O(n^2) 的搬移代价 */
        newCap = need;
    } else {
        /* 等比扩容：新容量 = max(need, cap_ * factor) */
        double grown = static_cast<double>(cap_) * growthFactor_;
        newCap = grown > static_cast<double>(need) ? static_cast<size_type>(grown) : need;
        if (newCap == cap_)
            newCap = need;
    }

    char *newData = new char[newCap];
    if (len_ > 0)
        std::memcpy(newData, data_, len_);
    newData[len_] = '\0';
    delete[] data_;
    data_ = newData;
    cap_ = newCap;
    ++reallocCount_;
}

void dsaac::MyString::reserve(size_type newCap)
{
    if (newCap <= cap_)
        return;

    char *newData = new char[newCap];
    if (len_ > 0)
        std::memcpy(newData, data_, len_);
    newData[len_] = '\0';
    delete[] data_;
    data_ = newData;
    cap_ = newCap;
    ++reallocCount_;
}

void dsaac::MyString::clear()
{
    len_ = 0;
    data_[0] = '\0';
}

void dsaac::MyString::append(const MyString &other)
{
    if (other.len_ == 0)
        return;
    ensureCapacity(len_ + other.len_ + 1);
    std::memcpy(data_ + len_, other.data_, other.len_);
    len_ += other.len_;
    data_[len_] = '\0';
}

void dsaac::MyString::append(const char *str)
{
    const std::size_t n = cstrLength(str);
    if (n == 0)
        return;
    ensureCapacity(len_ + n + 1);
    std::memcpy(data_ + len_, str, n);
    len_ += n;
    data_[len_] = '\0';
}

dsaac::MyString dsaac::MyString::substr(size_type pos, size_type n) const
{
    /*
     * 边界语义（与课程骨架保持一致）：
     *   - 非空串：pos >= length() 视为越界，抛 std::out_of_range；
     *   - 空串：任意 pos 都返回空串（没有合法的“起点”概念，但空结果有意义）；
     *   - 截取长度越过末尾时按末尾截断（pos + n > length()）。
     */
    if (len_ > 0 && pos >= len_)
        throw std::out_of_range("substr: pos out of range");

    const size_type start = (pos < len_) ? pos : len_;
    const size_type subLen = (n > len_ - start) ? (len_ - start) : n;
    return MyString(data_ + start, subLen);
}

std::size_t dsaac::MyString::find(const MyString &needle) const
{
    if (needle.len_ == 0)
        return 0;
    if (needle.len_ > len_)
        return npos;

    const size_type last = len_ - needle.len_;
    for (size_type i = 0; i <= last; ++i) {
        size_type j = 0;
        while (j < needle.len_ && data_[i + j] == needle.data_[j])
            ++j;
        if (j == needle.len_)
            return i;
    }
    return npos;
}

std::size_t dsaac::MyString::rfind(const MyString &needle) const
{
    if (needle.len_ == 0)
        return len_;
    if (needle.len_ > len_)
        return npos;

    const size_type last = len_ - needle.len_;
    for (size_type i = last + 1; i-- > 0;) {
        size_type j = 0;
        while (j < needle.len_ && data_[i + j] == needle.data_[j])
            ++j;
        if (j == needle.len_)
            return i;
    }
    return npos;
}

void dsaac::MyString::replace(const MyString &oldStr, const MyString &newStr)
{
    if (oldStr.len_ == 0)
        return;
    const size_type pos = find(oldStr);
    if (pos == npos)
        return;

    /* 先构造替换结果，避免 newStr 引用本对象时被中途改写 */
    MyString result;
    result.reserve(len_ - oldStr.len_ + newStr.len_ + 1);
    result.append(MyString(data_, pos));
    result.append(newStr);
    result.append(MyString(data_ + pos + oldStr.len_, len_ - pos - oldStr.len_));
    *this = result;
}

int dsaac::MyString::compare(const MyString &other) const
{
    const size_type common = std::min(len_, other.len_);
    for (size_type i = 0; i < common; ++i) {
        const unsigned char a = static_cast<unsigned char>(data_[i]);
        const unsigned char b = static_cast<unsigned char>(other.data_[i]);
        if (a < b)
            return -1;
        if (a > b)
            return 1;
    }
    if (len_ < other.len_)
        return -1;
    if (len_ > other.len_)
        return 1;
    return 0;
}

void dsaac::MyString::insert(size_type pos, const MyString &other)
{
    if (pos > len_)
        throw std::out_of_range("insert: pos out of range");
    if (other.len_ == 0)
        return;

    ensureCapacity(len_ + other.len_ + 1);
    /* 先把 pos 之后的旧内容整体后移，再在空位处写入 other */
    std::memmove(data_ + pos + other.len_, data_ + pos, len_ - pos);
    std::memcpy(data_ + pos, other.data_, other.len_);
    len_ += other.len_;
    data_[len_] = '\0';
}

void dsaac::MyString::erase(size_type pos, size_type n)
{
    if (pos > len_)
        throw std::out_of_range("erase: pos out of range");
    if (pos == len_ || n == 0)
        return;

    const size_type count = (n > len_ - pos) ? (len_ - pos) : n;
    std::memmove(data_ + pos, data_ + pos + count, len_ - pos - count);
    len_ -= count;
    data_[len_] = '\0';
}

char &dsaac::MyString::operator[](size_type index)
{
    return data_[index];
}

const char &dsaac::MyString::operator[](size_type index) const
{
    return data_[index];
}

bool dsaac::MyString::operator==(const MyString &other) const
{
    return compare(other) == 0;
}

bool dsaac::MyString::operator!=(const MyString &other) const
{
    return compare(other) != 0;
}

bool dsaac::MyString::operator<(const MyString &other) const
{
    return compare(other) < 0;
}

bool dsaac::MyString::operator>(const MyString &other) const
{
    return compare(other) > 0;
}

dsaac::MyString dsaac::MyString::operator+(const MyString &other) const
{
    MyString result(*this);
    result.append(other);
    return result;
}

dsaac::MyString &dsaac::MyString::operator+=(const MyString &other)
{
    append(other);
    return *this;
}

void dsaac::MyString::setGrowthFactor(double factor)
{
    /* 只接受 0（+1 语义）或 >= 1 的等比因子，避免容量不增长 */
    if (factor <= 0.0) {
        growthFactor_ = 0.0;
    } else if (factor >= 1.0) {
        growthFactor_ = factor;
    }
}

std::size_t dsaac::MyString::reallocationCount() const
{
    return reallocCount_;
}

/* END */