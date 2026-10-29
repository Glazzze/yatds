#ifndef ZUMA_CORE_GAME_HPP
#define ZUMA_CORE_GAME_HPP

#include "ball.hpp"
#include "track.hpp"
#include "waiting_queue.hpp"

#include "linked_stack.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace zuma {

/*
 * 撤销记录（命令模式，D3 选 A）。一条记录 = 完整恢复一步操作所需的全部信息。
 *
 * 核心难点是 SHOOT/REVERSE 撤销时的轨道回填。一次操作可能发生多轮
 * 消除（连锁），每轮消除的是"当前轨道"的一个连续段——各轮区间坐标
 * 基于当时的轨道状态，无法用单一区间表示。因此记录为**消除区间的
 * 栈序序列**（按发生顺序 push），撤销时**逆序**逐个把被消球串插回
 * 原区间左侧位置：
 *
 *   removed[i].left  第 i 轮消除段在"第 i 轮消除前轨道"中的左下标
 *   removed[i].balls 第 i 轮被消除的球颜色（位置升序）
 *
 * 撤销时对 i = n-1 … 0 依次执行"在当前轨道下标 left 处插入 balls 串"
 * —— 由于后发生的消除其区间基于更晚的轨道状态，逆序回填恰使每次
 * 插入的下标落在正确位置（见 design.md §5.2 的归纳证明）。最后再删
 * 掉发射的那颗球本身。
 */
enum OpKind {
    kOpShoot = 1,   // 发射（含炸弹/变色）
    kOpSwap = 2,    // 交换头两球
    kOpReverse = 3, // 倒退（反转区间）
};

struct UndoEntry {
    int op = 0;        // OpKind
    int pos = 0;       // SHOOT: 插入下标；REVERSE: 区间左端 i
    int extra = 0;     // SHOOT 变色球: 目标颜色；REVERSE: 区间右端 j；SWAP: 0
    Ball shotBall = 0; // SHOOT: 队列取出的原始球符号（变色球仍是 '?'，
                       // 撤销时原样 unshift 回队首；定色只作用于落位后的
                       // 普通球，不写回该字段）
    struct RemovedRun {
        int left;              // 本轮消除段在消除前轨道中的左下标
        std::vector<Ball> balls; // 被消除的球（位置升序）
    };
    std::vector<RemovedRun> removed; // 按消除发生顺序
    std::int64_t scoreBefore = 0; // 本步前的分数
    int movesBefore = 0;   // 本步前的步数
};

/*
 * 游戏状态与规则引擎（core 层，无任何 IO）。
 *
 * 不变量（design.md §4）：
 *   U1 轨道双向链表无哨兵，head_/tail_ 恒指端点或双双为 null
 *   U2 轨道上任意时刻不存在 ≥3 连同色段（插入/反转后必经 settle）
 *   U3 分数非负，仅消除事件按连锁倍率增加，UNDO 同步回滚
 *   U4 只有合法 SHOOT/SWAP/REVERSE 计步；UNDO 恢复，查询/帮助/非法不计步
 *
 * 状态机（D5 确认）：
 *   PLAYING → WIN（轨道清空）
 *           → LOSE（队列耗尽且轨道非空）
 *           → QUIT（主动退出）
 *   WIN/LOSE 后 UNDO 可回退；QUIT 是不可恢复的退出态。
 */
class Game {
public:
    /* 装载局面：颜色表、初始轨道、待发射队列、操作上限（0=不限）。
     * colors 仅含普通球颜色（大写字母），不含特殊球符号。
     * 装载后立即对轨道 settle（消除既有 ≥3 段，README 状态规则：
     * "轨道上出现连续 ≥3 同色即消除"，装载属于"出现"）。装载消除
     * 不计分、不产生事件、不可撤销（属局面初始化）。 */
    Game(const std::string &colors, const std::string &track,
         const std::string &queue, int limit);

    enum Status { kPlaying, kWin, kLose, kQuit };

    Status status() const { return status_; }
    std::int64_t score() const { return score_; }
    int moves() const { return moves_; }
    int limit() const { return limit_; }
    bool undoAvailable() const { return !undoStack_.empty(); }
    const DoublyLinkedTrack &track() const { return track_; }
    const RingQueue<Ball> &queue() const { return queue_; }
    const std::string &colors() const { return colors_; }

    /* 轨道快照（渲染与撤销比对用）。O(n)。 */
    std::string trackString() const;

    /* 队列快照：待发射的球（队首在最左）。O(size)。 */
    std::string queueString() const;

    /* ---------- 消除事件（输出日志与撤销记录的数据源） ---------- */

    struct ElimEvent {
        int round;         // 第几轮消除（1=首消，2+=连锁）
        int left;          // 本轮消除段在"本轮消除前轨道"中的左下标
        int count;         // 本轮消除球数
        std::int64_t points; // 本轮得分 = count × 10 × 2^(round-1)，饱和于 kScoreCap
        std::string balls; // 本轮被消除的球颜色
        std::string trackAfter; // 本轮消除后的轨道快照（协议输出用）
    };

    /* 发射：取队首球插入轨道下标 pos（变色球须给 color，普通球 color
     * 忽略）。返回消除事件序列（空 = 无消除）。
     * 非法参数（pos 越界 / 队列空 / 变色球未定色 / 终局态）抛异常，
     * 由 driver 捕获并作为非法指令报告（不改游戏状态）。 */
    std::vector<ElimEvent> shoot(int pos, int color = 0);

    /* 交换头两球（SWAP 道具）。队列 <2 球抛 out_of_range。 */
    void swap();

    /* 倒退：反转轨道闭区间 [i, j]（REVERSE 道具），反转后 settle。
     * 区间非法（i<0 / j<i / j 越界）抛 out_of_range。
     * 返回消除事件序列（可为空）。 */
    std::vector<ElimEvent> reverse(int i, int j);

    /* 提示：当前队首球的最优插入位置（HINT 道具，纯查询不改状态）。
     * 最优 = 插入后立即消除球数最大；无任何可消除位置返回 -1；
     * 队首为变色球返回 -1（定色自由度太大，不提示）。 */
    int hint() const;

    /* 撤销上一步（含连锁消除的完整回滚，可从 WIN/LOSE 回退）。
     * QUIT 态或无可撤销记录返回 false。 */
    bool undo();

    /* 主动退出（QUIT）。 */
    void quit() { status_ = kQuit; }

    /* 便捷构造（轨道长度上限，校验用）。 */
    static constexpr std::size_t kMaxTrack = 1000;
    static constexpr std::size_t kMaxQueue = 2000;

    /* 分数上限 = 2^53 - 1（JavaScript Number.MAX_SAFE_INTEGER）。C++ 与
     * 网页两端在每轮得分与总分累加时都饱和到该值，保证同一局面的分数
     * 逐字节一致（PROTOCOL.md §3.5）。 */
    static constexpr std::int64_t kScoreCap = 9007199254740991LL;

private:
    /*
     * 消除/连锁核心：以 probe 所在球为中心检查消除并级联至稳定。
     *
     * 普通球：找 probe 所在同色段 [segLeft..segRight]，len >= 3 则删除；
     *   删除后拼缝两侧若同色则连接成新段，继续以"拼缝右侧首节点"为
     *   probe 检查（等价于以拼缝为中心），直至某轮 len < 3。
     * 炸弹球（仅当轮内出现）：消除以炸弹为中心的至多 3 连球（无视
     *   颜色；两侧不足时消到端点，至少消炸弹自身），后续与普通消除
     *   相同（连锁沿拼缝、受三同色规则约束）。
     *
     * events 记录每轮 ElimEvent；undoEntry.removed 记录每轮区间与
     * 球序列（撤销逆序回填用）。
     */
    void settleFrom(TrackNode *probe, std::vector<ElimEvent> &events,
                    UndoEntry &undoEntry);

    /* 找 node 所在同色连续段的两端节点（node 须为普通球）。 */
    void findColorRun(TrackNode *node, TrackNode *&segLeft,
                      TrackNode *&segRight) const;

    /* 指针值比较判断节点是否仍在轨道（可能已被 settle 删除）。 */
    bool nodeAlive(const TrackNode *node) const;

    /* 段内节点计数（[segLeft, segRight]）。 */
    static std::size_t runLength(const TrackNode *segLeft,
                                 const TrackNode *segRight);

    /* 删除 [segLeft..segRight] 共 len 球，记事件、记撤销区间、更新分
     * 数；返回删除后拼缝右侧首节点（null = 删到尾），作为连锁 probe。 */
    TrackNode *eliminateRun(TrackNode *segLeft, TrackNode *segRight,
                            std::size_t len, int round,
                            std::vector<ElimEvent> &events,
                            UndoEntry &undoEntry);

    /* 计分：count 球、第 round 轮 → count × 10 × 2^(round-1)。
     *
     * 分数上限 kScoreCap = 2^53 - 1（JavaScript Number.MAX_SAFE_INTEGER，
     * 公开于类常量）：网页端用 Number 表示分数，超过该值后整数运算不再
     * 精确。C++ 与 JS 两端在**每轮得分**和**总分累加**时都饱和到该上限，
     * 保证同一局面的分数与事件 points 完全一致（PROTOCOL.md §3.5）。
     *
     * 深连锁下 2^(round-1) 会超出 32 位 int（round ≥ 28 即溢出），故
     * 中间量与结果统一用 int64，并在移位前判饱和。 */
    static std::int64_t satMul(std::int64_t a, std::int64_t b)
    {
        if (a <= 0 || b <= 0)
            return 0;
        if (a > kScoreCap / b)
            return kScoreCap;
        return a * b;
    }

    static std::int64_t satAdd(std::int64_t a, std::int64_t b)
    {
        if (b >= kScoreCap - a)
            return kScoreCap;
        return a + b;
    }

    static std::int64_t elimPoints(std::int64_t count, int round)
    {
        const std::int64_t base = satMul(count, 10);
        if (round <= 1)
            return base;
        // 2^(round-1) ≥ 2^53 时必然饱和（cap = 2^53 - 1）
        if (round - 1 >= 53)
            return kScoreCap;
        return satMul(base, static_cast<std::int64_t>(1) << (round - 1));
    }

    /* 根据轨道与队列重判终局。 */
    void refreshStatus();

    std::string colors_;        // 合法普通球颜色集合（如 "RGBY"）
    DoublyLinkedTrack track_;   // 轨道（D2）
    RingQueue<Ball> queue_;     // 待发射队列（D3）
    LinkedStack<UndoEntry> undoStack_; // 项目内自包含的链式撤销栈
    std::int64_t score_ = 0;    // 总分（U3，饱和于 kScoreCap）
    int moves_ = 0;             // 已执行步数（U4）
    int limit_ = 0;             // 操作上限（0=不限）
    Status status_ = kPlaying;  // 状态机
};

} // namespace zuma

#endif // ZUMA_CORE_GAME_HPP
