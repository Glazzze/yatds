#include "track.hpp"

#include <stdexcept>
#include <string>

namespace zuma {

/*
 * 轨道（双向链表）实现。
 *
 * 结构约定：无哨兵节点。head_ = 左端首节点，tail_ = 右端尾节点，
 * 空表时两者均为 nullptr。头尾指针使两端插入 O(1)；中间插入依赖
 * at() 定位（O(min(pos, size-pos))），定位后 O(1)。
 */

DoublyLinkedTrack::DoublyLinkedTrack(const char *track)
{
    for (const char *p = track; p && *p; ++p)
        insert_at(size_, *p); // 尾插建链
}

DoublyLinkedTrack::~DoublyLinkedTrack()
{
    TrackNode *cur = head_;
    while (cur) {
        TrackNode *next = cur->next;
        delete cur;
        cur = next;
    }
    head_ = tail_ = nullptr;
    size_ = 0;
}

TrackNode *DoublyLinkedTrack::insert_at(std::size_t pos, Ball b)
{
    if (pos > size_)
        throw std::out_of_range("插入位置越界");

    TrackNode *node = new TrackNode(b);

    if (pos == 0) { // 头插
        node->next = head_;
        if (head_)
            head_->prev = node;
        head_ = node;
        if (!tail_)
            tail_ = node;
    } else if (pos == size_) { // 尾插
        node->prev = tail_;
        tail_->next = node;
        tail_ = node;
    } else { // 中间插（pos < size，at() 取现位于 pos 的节点，插到其前）
        TrackNode *succ = at(pos);
        node->prev = succ->prev;
        node->next = succ;
        succ->prev->next = node;
        succ->prev = node;
    }
    ++size_;
    return node;
}

void DoublyLinkedTrack::erase(TrackNode *node)
{
    if (!node || size_ == 0)
        throw std::invalid_argument("erase: 非法节点");

    if (node->prev)
        node->prev->next = node->next;
    else
        head_ = node->next;
    if (node->next)
        node->next->prev = node->prev;
    else
        tail_ = node->prev;

    --size_;
    delete node;
}

TrackNode *DoublyLinkedTrack::at(std::size_t pos)
{
    return const_cast<TrackNode *>(
        static_cast<const DoublyLinkedTrack *>(this)->at(pos));
}

const TrackNode *DoublyLinkedTrack::at(std::size_t pos) const
{
    if (pos >= size_)
        throw std::out_of_range("下标越界");

    // 从较近的一端走，O(min(pos, size-1-pos))；随机下标定位仍是 O(n)，
    // 不属于 O(1) 摊销操作。
    if (pos < size_ - pos) {
        const TrackNode *cur = head_;
        for (std::size_t i = 0; i < pos; ++i)
            cur = cur->next;
        return cur;
    }
    const TrackNode *cur = tail_;
    for (std::size_t i = size_ - 1; i > pos; --i)
        cur = cur->prev;
    return cur;
}

std::size_t DoublyLinkedTrack::index_of(const TrackNode *node) const
{
    std::size_t idx = 0;
    for (const TrackNode *cur = head_; cur; cur = cur->next, ++idx)
        if (cur == node)
            return idx;
    throw std::invalid_argument("index_of: 节点不属于本轨道");
}

void DoublyLinkedTrack::reverse_range(std::size_t i, std::size_t j)
{
    if (i > j || j >= size_)
        throw std::out_of_range("反转区间越界");

    /* 经典三指针逐节点逆置 [i, j] 内部的 prev/next，区间外两次拼接。
     * 区间内节点数 L = j-i+1，每个节点交换 prev/next，共 L 次。 */
    TrackNode *before = (i == 0) ? nullptr : at(i - 1);
    TrackNode *first = at(i);          // 反转前区间首（反转后区间尾）
    TrackNode *after = (j + 1 == size_) ? nullptr : at(j + 1);

    TrackNode *prev = before;
    TrackNode *cur = first;
    for (std::size_t k = i; k <= j; ++k) {
        TrackNode *next = cur->next;
        cur->next = prev;
        cur->prev = next;
        prev = cur;
        cur = next;
    }
    // prev 现指向反转前区间尾（反转后区间首）
    TrackNode *newFirst = prev;

    if (before)
        before->next = newFirst;
    else
        head_ = newFirst;
    newFirst->prev = before;

    if (after) {
        after->prev = first;
        first->next = after;
    } else {
        tail_ = first;
        first->next = nullptr;
    }
}

bool DoublyLinkedTrack::links_valid() const
{
    if (size_ == 0)
        return head_ == nullptr && tail_ == nullptr;

    // 正向走：数节点、验 prev 回链
    std::size_t count = 0;
    const TrackNode *prev = nullptr;
    for (const TrackNode *cur = head_; cur; cur = cur->next) {
        if (cur->prev != prev)
            return false;
        prev = cur;
        ++count;
        if (count > size_)
            return false;
    }
    if (count != size_ || prev != tail_)
        return false;
    return true;
}

TrackNode *DoublyLinkedTrack::insert_before(TrackNode *anchor,
                                            const char *balls)
{
    TrackNode *first = nullptr;
    for (const char *p = balls; p && *p; ++p) {
        TrackNode *node = new TrackNode(*p);
        if (!anchor) { // 尾插
            node->prev = tail_;
            if (tail_)
                tail_->next = node;
            else
                head_ = node;
            tail_ = node;
        } else {
            node->prev = anchor->prev;
            node->next = anchor;
            if (anchor->prev)
                anchor->prev->next = node;
            else
                head_ = node;
            anchor->prev = node;
        }
        if (!first)
            first = node;
        ++size_;
    }
    return first;
}

TrackNode *DoublyLinkedTrack::erase_run(TrackNode *start, std::size_t k)
{
    if (!start || k == 0 || k > size_)
        throw std::invalid_argument("erase_run: 非法参数");

    // 先验证链上确有 k 个节点，再开始删除：非法长度必须在任何修改
    // 之前拒绝，保证容器不发生"删掉一半后抛异常"的部分变更。
    TrackNode *cur = start;
    for (std::size_t i = 0; i < k; ++i) {
        if (!cur)
            throw std::invalid_argument("erase_run: 长度超出链尾");
        cur = cur->next;
    }

    TrackNode *before = start->prev;
    cur = start;
    for (std::size_t i = 0; i < k; ++i) {
        TrackNode *next = cur->next;
        delete cur;
        cur = next;
    }
    size_ -= k;

    if (before)
        before->next = cur;
    else
        head_ = cur;
    if (cur)
        cur->prev = before;
    else
        tail_ = before;
    return cur; // 区间右侧首节点（可能为 null）
}

} // namespace zuma
