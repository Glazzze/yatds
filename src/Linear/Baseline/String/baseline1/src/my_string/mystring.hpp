#ifndef DSAAC_MYSTRING_HPP
#define DSAAC_MYSTRING_HPP

#include <cstddef>

namespace dsaac {

/*
 * 顺序存储的字符串（基于动态数组）。
 *
 * 数据布局：data_ 指向一段连续堆内存，保存 len_ 个字符，并以 '\0' 结尾；
 * cap_ 是这段内存的字符槽位总数（含结尾 '\0'），因此恒有 cap_ >= len_ + 1。
 * 当 cap_ 不足以容纳新内容时按 growthFactor_ 扩容（默认 2 倍）。
 */
class MyString {
public:
    using size_type = std::size_t;

    /* 未找到时的返回值，语义与 std::string::npos 一致 */
    static const size_type npos = static_cast<size_type>(-1);

    MyString();
    explicit MyString(const char *str);
    MyString(const char *str, size_type count);
    ~MyString();

    MyString(const MyString &other);
    MyString &operator=(const MyString &other);
    MyString(MyString &&other) noexcept;
    MyString &operator=(MyString &&other) noexcept;

    /* ---------- 基础操作（课程要求） ---------- */
    size_type length() const;
    MyString substr(size_type pos, size_type n) const;
    void append(const MyString &other);

    /* ---------- 参照 std::string 的扩展功能 ---------- */
    size_type size() const;
    bool empty() const;
    size_type capacity() const;
    const char *c_str() const;

    /* 预留至少 newCap 个字符槽位（含结尾 '\0'），用于避免频繁扩容 */
    void reserve(size_type newCap);
    /* 清空内容，但保留已分配容量 */
    void clear();

    /* 返回首次/最后一次出现的下标；不存在返回 npos；空模式返回 0 */
    size_type find(const MyString &needle) const;
    size_type rfind(const MyString &needle) const;

    /* 将首次出现的 oldStr 替换为 newStr；oldStr 为空或不存在时不变 */
    void replace(const MyString &oldStr, const MyString &newStr);

    /* 字典序比较：相等 0，小于 -1，大于 1 */
    int compare(const MyString &other) const;

    /* 在下标 pos 处插入 other；pos > length() 抛 std::out_of_range */
    void insert(size_type pos, const MyString &other);
    /* 删除从 pos 开始的至多 n 个字符；pos > length() 抛 std::out_of_range */
    void erase(size_type pos, size_type n);

    void append(const char *str);

    char &operator[](size_type index);
    const char &operator[](size_type index) const;

    bool operator==(const MyString &other) const;
    bool operator!=(const MyString &other) const;
    bool operator<(const MyString &other) const;
    bool operator>(const MyString &other) const;

    MyString operator+(const MyString &other) const;
    MyString &operator+=(const MyString &other);

    /* ---------- 供性能实验使用的观测接口 ---------- */
    /* 设置扩容因子：2.0 表示 2 倍扩容，1.5 表示 1.5 倍，0 表示每次只扩到刚好够用（+1 语义） */
    void setGrowthFactor(double factor);
    /* 返回累计发生内存重新分配的次数的计数（供扩容策略实验使用） */
    size_type reallocationCount() const;

private:
    /* 保证 cap_ >= need（need 已包含结尾 '\0' 的槽位） */
    void ensureCapacity(size_type need);
    /* 把其他对象的数据复制到本对象（调用方负责先释放旧内存） */
    void copyFrom(const MyString &other);

    char *data_;
    size_type len_;
    size_type cap_;
    double growthFactor_;
    size_type reallocCount_;
};

} // namespace dsaac

// 为了方便使用，将类型与常量暴露在全局（也可按需不暴露）
using MyString = dsaac::MyString;

#endif // DSAAC_MYSTRING_HPP