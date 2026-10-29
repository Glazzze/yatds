#ifndef ZUMA_CORE_TRACK_HPP
#define ZUMA_CORE_TRACK_HPP

#include "ball.hpp"

#include <cstddef>

namespace zuma {

/*
 * 彩球轨道 —— 双向链表（D2 选 A，本项目核心自实现数据结构）。
 *
 * 设计动机（见 docs/design.md §2）：
 *   - 插入/删除定位后 O(1)：祖玛的核心操作是"插入一球 → 可能删除一段连续球"，
 *     数组实现每次插入 O(n) 且删除一段后整体搬移 O(n)；
 *   - 消除后两段拼接 = O(1) 链接操作；
 *   - 连锁检查沿拼缝向两侧走 = 沿 prev/next 指针前进，天然匹配；
 *   - 倒退（反转）道具在链表上是经典三指针逆置，内部 O(L)，含下标定位 O(n+L)。
 *
 * 节点直接持有 Ball 值（结构简单，无需在节点层再加一层抽象）。
 */
struct TrackNode {
    Ball ball;
    TrackNode *prev;
    TrackNode *next;

    explicit TrackNode(Ball b, TrackNode *p = nullptr, TrackNode *n = nullptr)
        : ball(b), prev(p), next(n) {}
};

class DoublyLinkedTrack {
public:
    /*
     * 由颜色序列构造轨道（装载后是否触发消除由 Game 决定，
     * 本类只做容器，不含游戏规则 —— core 内分层：容器 / 规则）。
     */
    explicit DoublyLinkedTrack(const char *track); // C 字符串，逐字符建球
    DoublyLinkedTrack(const DoublyLinkedTrack &) = delete;            // 禁拷贝
    DoublyLinkedTrack &operator=(const DoublyLinkedTrack &) = delete; // 禁赋值
    ~DoublyLinkedTrack();                                             // 释放全部节点

    /* ---------- 容量与遍历 ---------- */

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    TrackNode *head() { return head_; }             // 首节点（左端），空表为 nullptr
    TrackNode *tail() { return tail_; }             // 尾节点（右端）
    const TrackNode *head() const { return head_; }
    const TrackNode *tail() const { return tail_; }

    /* ---------- 位置语义（对齐 PROTOCOL.md §3：0-based，insert 前） ---------- */

    /* 在下标 pos 处插入（新球插入后位于下标 pos）。pos ∈ [0, size]。 */
    TrackNode *insert_at(std::size_t pos, Ball b);

    /* 删除节点 node（须为本轨道节点且非空指针）。 */
    void erase(TrackNode *node);

    /* 下标 → 节点。pos ∈ [0, size)。O(min(pos, size−1−pos))（走最近一端）。 */
    TrackNode *at(std::size_t pos);
    const TrackNode *at(std::size_t pos) const;

    /* 求节点下标。O(n)，仅供渲染/调试与撤销回填定位。 */
    std::size_t index_of(const TrackNode *node) const;

    /* 就地反转闭区间 [i, j]（倒退道具）。0-based，i <= j < size。 */
    void reverse_range(std::size_t i, std::size_t j);

    /* 重建链表前后的哨兵一致性检查（调试用，release 下 O(1) 空表检查）。 */
    bool links_valid() const;

    /* ---------- 供撤销回填：与消除配合的批量操作 ---------- */

    /* 把序列 balls 依次插入到 anchor 节点之前（anchor 可为 null = 尾插）。
     * 返回插入的第一个节点；空序列返回 nullptr。 */
    TrackNode *insert_before(TrackNode *anchor, const char *balls);

    /* 从 start 起沿 next 连续取 k 个节点删除（k >= 1，start 非空）。
     * 返回删除后区间右侧的第一个节点（可能为 null）。
     * 强异常安全：若 start 之后不足 k 个节点，先抛 invalid_argument，
     * 容器保持原状。供消除算法成段删除；删除前调用方需先记录被删球
     * 颜色（撤销回填用）。 */
    TrackNode *erase_run(TrackNode *start, std::size_t k);

private:
    TrackNode *head_ = nullptr; // 左端首节点（无哨兵：祖玛轨道两端频繁变化，哨兵收益低）
    TrackNode *tail_ = nullptr; // 右端尾节点（tail 使尾插 O(1)）
    std::size_t size_ = 0;
};

} // namespace zuma

#endif // ZUMA_CORE_TRACK_HPP
