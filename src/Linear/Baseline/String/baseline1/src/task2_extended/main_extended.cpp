/*
 * Baseline 1 小任务 2：扩展功能单元验证。
 *
 * 课程要求参照 std::string 为 MyString 增加至少 5 个功能。本程序对
 * find / rfind / replace / compare / insert / erase / clear / reserve /
 * operator[] / 关系运算符 / operator+ 等扩展逐一断言，并覆盖空串、
 * 越界、别名（self-replace）、尾部截断等边界。
 *
 * 全部通过时返回 0，否则返回 1。
 */
#include "../my_string/mystring.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int gPassed = 0;
int gFailed = 0;

void check(bool cond, const std::string &what)
{
    if (cond) {
        ++gPassed;
    } else {
        ++gFailed;
        std::cout << "[FAIL] " << what << '\n';
    }
}

void checkEq(const MyString &actual, const char *expected, const std::string &what)
{
    if (std::string(actual.c_str()) == std::string(expected)) {
        ++gPassed;
    } else {
        ++gFailed;
        std::cout << "[FAIL] " << what << "：期望 \"" << expected << "\"，实际 \""
                  << actual.c_str() << "\"\n";
    }
}

void checkEqInt(long long actual, long long expected, const std::string &what)
{
    if (actual == expected) {
        ++gPassed;
    } else {
        ++gFailed;
        std::cout << "[FAIL] " << what << "：期望 " << expected << "，实际 " << actual
                  << '\n';
    }
}

} // namespace

int main()
{
    /* ---------- 1. 基础构造与属性 ---------- */
    {
        MyString empty;
        check(empty.empty(), "默认构造为空串");
        checkEqInt(static_cast<long long>(empty.length()), 0, "默认构造长度为 0");
        check(std::string(empty.c_str()) == "", "默认构造 c_str 为 \"\"");

        MyString s("Hello");
        checkEqInt(static_cast<long long>(s.length()), 5, "const char* 构造长度");
        checkEqInt(static_cast<long long>(s.capacity()) >= 6 ? 1 : 0, 1,
                   "capacity 至少能容纳内容与结尾 '\\0'");
        checkEq(s, "Hello", "const char* 构造内容");
    }

    /* ---------- 2. 拷贝 / 赋值深拷贝 ---------- */
    {
        MyString a("abcdef");
        MyString b(a);
        b[0] = 'X';
        checkEq(a, "abcdef", "拷贝构造后修改副本不影响原串");
        checkEq(b, "Xbcdef", "拷贝构造出的副本可独立修改");

        MyString c;
        c = a;
        c[5] = 'Y';
        checkEq(a, "abcdef", "赋值后修改副本不影响原串");
        checkEq(c, "abcdeY", "赋值后副本内容正确");

        c = c; // 自赋值
        checkEq(c, "abcdeY", "自赋值安全");
    }

    /* ---------- 3. 拼接与扩容 ---------- */
    {
        MyString s("abc");
        for (int i = 0; i < 100; ++i)
            s.append(MyString("xy"));
        checkEqInt(static_cast<long long>(s.length()), 203, "连续 append 后长度正确");
        check(std::string(s.c_str()).size() == 203, "append 后 c_str 长度与 length 一致");
        check(s.reallocationCount() > 0, "append 触发过扩容");
        check(s.capacity() >= s.length() + 1, "扩容后容量满足不变式 cap >= len + 1");

        MyString t("tail");
        s.append(t);
        check(std::string(s.c_str()).substr(203) == "tail", "append 另一对象内容正确");

        MyString u;
        u.reserve(1000);
        const std::size_t before = u.reallocationCount();
        for (int i = 0; i < 500; ++i)
            u.append(MyString("z"));
        checkEqInt(static_cast<long long>(u.length()), 500, "reserve 后持续 append 长度正确");
        check(u.reallocationCount() == before, "reserve 足够容量后不再重新分配");
    }

    /* ---------- 4. substr 边界 ---------- */
    {
        MyString s("HelloWorld");
        checkEq(s.substr(0, 5), "Hello", "substr 从头部截取");
        checkEq(s.substr(5, 5), "World", "substr 从中部截取到末尾");
        checkEq(s.substr(0, 100), "HelloWorld", "substr 长度越界按结尾截断");
        checkEq(s.substr(9, 0), "", "substr 在末尾前取长度为 0 的空串");

        bool threw = false;
        try {
            (void)s.substr(10, 0);
        } catch (const std::out_of_range &) {
            threw = true;
        }
        check(threw, "非空串 substr pos == length 抛 out_of_range（即使长度为 0）");

        threw = false;
        try {
            (void)s.substr(11, 1);
        } catch (const std::out_of_range &) {
            threw = true;
        }
        check(threw, "非空串 substr pos > length 抛 out_of_range");

        MyString e;
        checkEq(e.substr(0, 0), "", "空串 substr(0,0) 返回空串");
        checkEq(e.substr(5, 3), "", "空串 substr 任意 pos 返回空串");
    }

    /* ---------- 5. find / rfind ---------- */
    {
        MyString s("abcabcabc");
        checkEqInt(static_cast<long long>(s.find(MyString("abc"))), 0, "find 首次出现");
        checkEqInt(static_cast<long long>(s.find(MyString("cab"))), 2, "find 内部出现");
        checkEqInt(static_cast<long long>(s.find(MyString("xyz"))),
                   static_cast<long long>(MyString::npos), "find 不存在返回 npos");
        checkEqInt(static_cast<long long>(s.find(MyString(""))), 0, "find 空模式返回 0");
        checkEqInt(static_cast<long long>(s.rfind(MyString("abc"))), 6, "rfind 最后一次出现");
        checkEqInt(static_cast<long long>(s.rfind(MyString("abcabc"))), 3,
                   "rfind 重叠后缀出现");
        checkEqInt(static_cast<long long>(s.rfind(MyString(""))),
                   static_cast<long long>(s.length()), "rfind 空模式返回 length");
        checkEqInt(static_cast<long long>(s.rfind(MyString("xyz"))),
                   static_cast<long long>(MyString::npos), "rfind 不存在返回 npos");
    }

    /* ---------- 6. replace ---------- */
    {
        MyString s("Hello World");
        s.replace(MyString("World"), MyString("String"));
        checkEq(s, "Hello String", "replace 首次出现");

        MyString t("abcabc");
        t.replace(MyString("abc"), MyString("X"));
        checkEq(t, "Xabc", "replace 只替换首次出现");

        t.replace(MyString("zzz"), MyString("Y"));
        checkEq(t, "Xabc", "replace 未找到时不变");

        t.replace(MyString(""), MyString("Y"));
        checkEq(t, "Xabc", "replace 空 old 不变");

        MyString u("abc");
        u.replace(MyString("abc"), MyString("abcabc"));
        checkEq(u, "abcabc", "replace new 比 old 长");

        MyString v("xabcx");
        v.replace(MyString("abc"), MyString(""));
        checkEq(v, "xx", "replace new 为空即删除");

        /* 别名：new 引用本对象本身（"aa" 的首个 "a" 替换为 "aa" → "aaa"） */
        MyString w("aa");
        w.replace(MyString("a"), w);
        checkEq(w, "aaa", "replace 的 new 引用本对象时结果正确");
    }

    /* ---------- 7. compare / 关系运算符 ---------- */
    {
        MyString a("abc");
        MyString b("abd");
        MyString c("abc");
        MyString d("ab");

        checkEqInt(a.compare(c), 0, "compare 相等");
        checkEqInt(a.compare(b), -1, "compare 小于");
        checkEqInt(b.compare(a), 1, "compare 大于");
        checkEqInt(a.compare(d), 1, "compare 前缀更短时更小");

        check(a == c, "operator== 相等");
        check(a != b, "operator!= 不相等");
        check(a < b, "operator< 字典序");
        check(b > a, "operator> 字典序");
        check(d < a, "operator< 前缀关系");
    }

    /* ---------- 8. insert / erase ---------- */
    {
        MyString s("HelloWorld");
        s.insert(5, MyString(" "));
        checkEq(s, "Hello World", "insert 中间插入");
        s.insert(0, MyString(">>"));
        checkEq(s, ">>Hello World", "insert 头部插入");
        s.insert(s.length(), MyString("<<"));
        checkEq(s, ">>Hello World<<", "insert 尾部插入（pos == length）");

        bool threw = false;
        try {
            s.insert(s.length() + 1, MyString("!"));
        } catch (const std::out_of_range &) {
            threw = true;
        }
        check(threw, "insert pos > length 抛 out_of_range");

        s.erase(0, 2);
        checkEq(s, "Hello World<<", "erase 头部删除");
        s.erase(5, 1);
        checkEq(s, "HelloWorld<<", "erase 中间删除");
        s.erase(10, 100);
        checkEq(s, "HelloWorld", "erase 长度越界按结尾截断");
        s.erase(s.length(), 1);
        checkEq(s, "HelloWorld", "erase pos == length 无操作");

        threw = false;
        try {
            s.erase(s.length() + 1, 1);
        } catch (const std::out_of_range &) {
            threw = true;
        }
        check(threw, "erase pos > length 抛 out_of_range");
    }

    /* ---------- 9. clear / operator+ / operator+= ---------- */
    {
        MyString s("abc");
        const std::size_t capBefore = s.capacity();
        s.clear();
        check(s.empty(), "clear 后为空串");
        checkEqInt(static_cast<long long>(s.length()), 0, "clear 后长度为 0");
        check(s.capacity() == capBefore, "clear 保留容量");

        MyString a("Hello");
        MyString b(" World");
        MyString c = a + b;
        checkEq(c, "Hello World", "operator+ 返回拼接结果");
        checkEq(a, "Hello", "operator+ 不修改左操作数");
        checkEq(b, " World", "operator+ 不修改右操作数");

        a += b;
        checkEq(a, "Hello World", "operator+= 原地拼接");
    }

    /* ---------- 10. 扩容策略观测接口 ---------- */
    {
        MyString s;
        s.setGrowthFactor(0.0); // +1 语义
        for (int i = 0; i < 10; ++i)
            s.append(MyString("a"));
        checkEqInt(static_cast<long long>(s.reallocationCount()), 10,
                   "+1 扩容策略每次追加都重新分配");
        checkEqInt(static_cast<long long>(s.length()), 10, "+1 扩容后内容长度正确");

        MyString t;
        t.setGrowthFactor(2.0);
        for (int i = 0; i < 100; ++i)
            t.append(MyString("a"));
        check(t.reallocationCount() <= 8, "2 倍扩容的重分配次数为对数级");
        checkEqInt(static_cast<long long>(t.length()), 100, "2 倍扩容后内容长度正确");
    }

    std::cout << "扩展功能单元验证：通过 " << gPassed << " 项，失败 " << gFailed << " 项\n";
    return gFailed == 0 ? 0 : 1;
}