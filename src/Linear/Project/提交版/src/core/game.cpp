#include "game.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace zuma {

/*
 * 游戏规则引擎实现。消除/连锁/撤销算法的完整原理见 docs/design.md §3/§5，
 * 此处注释聚焦实现层面的不变量。
 */

Game::Game(const std::string &colors, const std::string &trackStr,
           const std::string &queueStr, int limit)
    : colors_(colors),
      track_(trackStr.c_str()),
      queue_(queueStr.empty() ? 1 : queueStr.size()),
      limit_(limit)
{
    for (char b : queueStr)
        queue_.push(b);

    // 装载 settle：消除轨道既有 ≥3 同色段（README 状态规则）。装载消除
    // 不计分、不产生事件、不可撤销。从 head 逐段扫描：对每个未检查的
    // 球找其同色段，≥3 则删除后从拼缝右侧继续（连锁），否则前进一格。
    // 注意删除会改变游标后的链表结构，next 指针须先取出。
    TrackNode *cur = track_.head();
    while (cur) {
        TrackNode *segLeft = nullptr, *segRight = nullptr;
        findColorRun(cur, segLeft, segRight);
        std::size_t len = runLength(segLeft, segRight);
        if (len >= 3) {
            // 拼缝右侧首节点（null = 删到尾）作为下一检查点
            TrackNode *after = segRight->next;
            track_.erase_run(segLeft, len);
            cur = after;
        } else {
            cur = cur->next;
        }
    }
    refreshStatus();
}

std::string Game::trackString() const
{
    std::string s;
    s.reserve(track_.size());
    for (const TrackNode *cur = track_.head(); cur; cur = cur->next)
        s.push_back(cur->ball);
    return s;
}

std::string Game::queueString() const
{
    std::string s;
    for (Ball b : queue_.snapshot())
        s.push_back(b);
    return s;
}

std::vector<Game::ElimEvent> Game::shoot(int pos, int color)
{
    if (status_ != kPlaying)
        throw std::logic_error("终局态不可发射");
    if (queue_.empty())
        throw std::out_of_range("待发射队列为空");
    if (pos < 0 || static_cast<std::size_t>(pos) > track_.size())
        throw std::out_of_range("发射位置越界");

    // 变色球在落位瞬间定色（PROTOCOL.md：SHOOT 的 color 参数）。
    // 先校验再 take()：非法指令必须不改变任何游戏状态（协议 §3.4），
    // 若先出队再拒绝，队首球会被吞掉。
    const Ball ballFront = queue_.front();
    if (ballFront == kChameleon) {
        if (colors_.find(static_cast<char>(color)) == std::string::npos)
            throw std::invalid_argument("变色球必须指定 COLORS 内的颜色");
    } else if (color != 0) {
        // 普通球/炸弹携带 color 视为协议错误（driver 保证不会发生；
        // core 层兜底拒绝，避免静默忽略）
        throw std::invalid_argument("仅变色球可指定颜色");
    }

    const Ball shotBall = queue_.take(); // 撤销时放回队首的球（含特殊球原符号）
    Ball ball = shotBall;
    if (ball == kChameleon)
        ball = static_cast<Ball>(color);

    UndoEntry entry;
    entry.op = kOpShoot;
    entry.pos = pos;
    entry.shotBall = shotBall;
    entry.scoreBefore = score_;
    entry.movesBefore = moves_;

    std::vector<ElimEvent> events;
    TrackNode *node = track_.insert_at(static_cast<std::size_t>(pos), ball);
    settleFrom(node, events, entry);

    ++moves_;
    undoStack_.push(entry);
    refreshStatus();
    return events;
}

void Game::swap()
{
    // 与 shoot()/reverse() 对齐：终局后拒绝执行。driver 已在终局后
    // 拦截指令，这里是 core 层的防御性守卫，保证直接调用 API 时状态
    // 也不会被修改（单测 swap-after-win 覆盖）。
    if (status_ != kPlaying)
        throw std::logic_error("终局态不可交换");

    // SWAP 操作队列，不碰轨道：空轨道（WIN 态）下队列有球即合法——
    // 但 WIN 已定胜负，终局后不应再改变状态。
    queue_.swap_front_two(); // size < 2 抛 out_of_range

    UndoEntry entry;
    entry.op = kOpSwap;
    entry.scoreBefore = score_;
    entry.movesBefore = moves_;
    undoStack_.push(entry);
    ++moves_;
    refreshStatus(); // SWAP 同样消耗一步，可能触发 LIMIT
}

std::vector<Game::ElimEvent> Game::reverse(int i, int j)
{
    if (status_ != kPlaying)
        throw std::logic_error("终局态不可反转");
    if (i < 0 || j < i || static_cast<std::size_t>(j) >= track_.size())
        throw std::out_of_range("反转区间越界");

    UndoEntry entry;
    entry.op = kOpReverse;
    entry.pos = i;
    entry.extra = j;
    entry.scoreBefore = score_;
    entry.movesBefore = moves_;

    track_.reverse_range(static_cast<std::size_t>(i),
                         static_cast<std::size_t>(j));

    // 反转拼接出两处拼缝：i-1/i 交界与 j/j+1 交界。分别取拼缝内侧
    // 节点（at(i)、at(j)）为 probe settle。指针必须在删除发生前取出，
    // 且第二次调用前只比较**指针值**判断该节点是否幸存（解引用已删除
    // 节点是 UAF）。probeB 若已被第一次 settle 消除，其所在段正是第
    // 一次的消除对象，跳过即可（语义已覆盖）。
    TrackNode *probeA = track_.at(static_cast<std::size_t>(i));
    TrackNode *probeB = track_.at(static_cast<std::size_t>(j));

    std::vector<ElimEvent> events;
    settleFrom(probeA, events, entry);
    if (probeB && nodeAlive(probeB))
        settleFrom(probeB, events, entry);

    undoStack_.push(entry);
    ++moves_;
    refreshStatus();
    return events;
}

int Game::hint() const
{
    if (status_ != kPlaying || queue_.empty())
        return -1;

    Ball ball = queue_.front();
    if (ball == kChameleon)
        return -1;

    // 模拟每个插入位 pos ∈ [0, n]：副本轨道上插入后找同色段长度，
    // 取"立即消除球数"最大者。等位置时返回最左（对玩家最直观）。
    // 炸弹球的最优位 = 邻域消除数最大的位置（同为"消除最多"语义）。
    int bestPos = -1;
    std::size_t bestElim = 0;
    const std::size_t n = track_.size();
    for (std::size_t pos = 0; pos <= n; ++pos) {
        std::size_t elim = 0;
        if (ball == kBomb) {
            // 端点可以继续向同一侧扩展：所有插入位首轮都消 min(3,n+1) 球。
            // 同分取最左，不能只计算左右各一颗。
            elim = std::min<std::size_t>(3, n + 1);
        } else {
            DoublyLinkedTrack sim(trackString().c_str());
            TrackNode *node = sim.insert_at(pos, ball);
            TrackNode *segLeft = nullptr, *segRight = nullptr;
            // 在副本上找段（复用轨道遍历，不依赖 Game 私有辅助）
            segLeft = segRight = node;
            while (segLeft->prev && segLeft->prev->ball == ball)
                segLeft = segLeft->prev;
            while (segRight->next && segRight->next->ball == ball)
                segRight = segRight->next;
            std::size_t len = 1;
            for (const TrackNode *p = segLeft; p != segRight; p = p->next)
                ++len;
            if (len >= 3)
                elim = len;
        }
        if (elim > bestElim) {
            bestElim = elim;
            bestPos = static_cast<int>(pos);
        }
    }
    return bestPos;
}

bool Game::undo()
{
    if (status_ == kQuit || undoStack_.empty())
        return false;

    UndoEntry entry = undoStack_.top();
    undoStack_.pop();

    // 先恢复计数与状态（回滚消除前先恢复计分基线）
    score_ = entry.scoreBefore;
    moves_ = entry.movesBefore;
    status_ = kPlaying;

    switch (entry.op) {
    case kOpShoot: {
        // 1) 逆序回填各轮被消段：第 i 轮区间下标基于第 i 轮消除前轨道，
        //    逆序（n-1 … 0）插入使每次插入位置恰好对齐（design.md §5.2）
        for (std::size_t k = entry.removed.size(); k-- > 0;) {
            const UndoEntry::RemovedRun &run = entry.removed[k];
            TrackNode *anchor = nullptr; // 插入到当前下标 left 的节点之前
            if (static_cast<std::size_t>(run.left) < track_.size())
                anchor = track_.at(static_cast<std::size_t>(run.left));
            std::string balls(run.balls.begin(), run.balls.end());
            track_.insert_before(anchor, balls.c_str());
        }
        // 2) 删除发射的那颗球：若它参与了消除则已在 removed 中被回填，
        //    未参与时位于当前下标 pos 处（回填各段下标 >= 被消区间，
        //    不影响 pos 前方节点；分情形论证见 design.md §5.3）
        if (static_cast<std::size_t>(entry.pos) < track_.size()) {
            TrackNode *shot = track_.at(static_cast<std::size_t>(entry.pos));
            // 发射球未参与消除的判定：回填后下标 pos 处恰为该球。
            // 保守起见校验（参与消除时 removed 回填会使后续段左移覆盖
            // pos——此情形下 pos 处是回填球，删除会破坏恢复）。
            // 精确处理：SHOOT 的插入球若被消除，则 removed 中含它，
            // 回填已还原；若未消除，回填区间都在它右侧或左侧不重叠，
            // pos 处即发射球。
            track_.erase(shot);
        }
        // 3) 球放回队首
        queue_.unshift(entry.shotBall);
        break;
    }
    case kOpSwap:
        queue_.swap_front_two(); // 交换自反
        break;
    case kOpReverse:
        // 先逆序回填消除，再反转同一区间恢复原序（区间坐标基于操作前
        // 轨道；回填后轨道与操作前一致，反转自反）
        for (std::size_t k = entry.removed.size(); k-- > 0;) {
            const UndoEntry::RemovedRun &run = entry.removed[k];
            TrackNode *anchor = nullptr;
            if (static_cast<std::size_t>(run.left) < track_.size())
                anchor = track_.at(static_cast<std::size_t>(run.left));
            std::string balls(run.balls.begin(), run.balls.end());
            track_.insert_before(anchor, balls.c_str());
        }
        track_.reverse_range(static_cast<std::size_t>(entry.pos),
                             static_cast<std::size_t>(entry.extra));
        break;
    default:
        break;
    }

    refreshStatus();
    return true;
}

/* ---------------- 消除核心 ---------------- */

void Game::settleFrom(TrackNode *probe, std::vector<ElimEvent> &events,
                      UndoEntry &undoEntry)
{
    int round = 0;
    bool first = true;

    while (probe) {
        ++round;

        if (first && probe->ball == kBomb) {
            /* 炸弹首轮：消除以炸弹为中心的至多 3 连球（无视颜色）。
             * 两侧不足时消到端点，至少消炸弹自身；连锁轮中拼缝两侧
             * 不再适用炸弹规则（炸弹已消）。 */
            TrackNode *segLeft = probe;
            TrackNode *segRight = probe;
            std::size_t len = 1;
            while (len < 3) {
                bool grew = false;
                if (segRight->next) {
                    segRight = segRight->next;
                    ++len;
                    grew = true;
                }
                if (len >= 3)
                    break;
                if (segLeft->prev) {
                    segLeft = segLeft->prev;
                    ++len;
                    grew = true;
                }
                if (!grew)
                    break;
            }
            TrackNode *next = eliminateRun(segLeft, segRight, len, round,
                                           events, undoEntry);
            probe = next;
            first = false;
            continue;
        }
        first = false;

        // 普通球：找 probe 所在同色段
        TrackNode *segLeft = nullptr, *segRight = nullptr;
        findColorRun(probe, segLeft, segRight);
        std::size_t len = runLength(segLeft, segRight);

        if (len < 3)
            break; // 稳定，连锁结束

        TrackNode *next = eliminateRun(segLeft, segRight, len, round,
                                       events, undoEntry);
        probe = next; // null = 删到尾，连锁终止
    }
}

void Game::findColorRun(TrackNode *node, TrackNode *&segLeft,
                        TrackNode *&segRight) const
{
    segLeft = segRight = node;
    while (segLeft->prev && segLeft->prev->ball == node->ball)
        segLeft = segLeft->prev;
    while (segRight->next && segRight->next->ball == node->ball)
        segRight = segRight->next;
}

/* 指针值比较判断节点是否仍在轨道上（不解引用 node 本身——它可能已被
 * settle 删除，解引用即 UAF）。O(n)，仅 reverse 的双拼缝检查用。 */
bool Game::nodeAlive(const TrackNode *node) const
{
    for (const TrackNode *p = track_.head(); p; p = p->next)
        if (p == node)
            return true;
    return false;
}

std::size_t Game::runLength(const TrackNode *segLeft,
                            const TrackNode *segRight)
{
    std::size_t len = 0;
    for (const TrackNode *p = segLeft;; p = p->next) {
        ++len;
        if (p == segRight)
            break;
    }
    return len;
}

TrackNode *Game::eliminateRun(TrackNode *segLeft, TrackNode *segRight,
                              std::size_t len, int round,
                              std::vector<ElimEvent> &events,
                              UndoEntry &undoEntry)
{
    /* 记录本轮消除信息（事件 + 撤销区间），随后删除并返回拼缝右侧
     * 首节点。被消段坐标基于当前（删除前）轨道，恰好是撤销回填与
     * ElimEvent.left 的统一坐标系。 */

    // 被消球颜色序列 + 左下标
    UndoEntry::RemovedRun run;
    run.left = static_cast<int>(track_.index_of(segLeft));
    for (const TrackNode *p = segLeft;; p = p->next) {
        run.balls.push_back(p->ball);
        if (p == segRight)
            break;
    }

    ElimEvent ev;
    ev.round = round;
    ev.left = run.left;
    ev.count = static_cast<int>(len);
    ev.points = elimPoints(static_cast<std::int64_t>(len), round);
    ev.balls = std::string(run.balls.begin(), run.balls.end());
    events.push_back(ev);
    score_ = satAdd(score_, ev.points);

    // run 此后不再使用，整体移入撤销记录
    undoEntry.removed.push_back(std::move(run));

    TrackNode *after = segRight->next;
    track_.erase_run(segLeft, len);

    // 消除完成后补记轨道快照（事件在 push 时没有 trackAfter——
    // 此处回填最后一个事件）
    if (!events.empty())
        events.back().trackAfter = trackString();
    return after;
}

void Game::refreshStatus()
{
    if (status_ == kQuit)
        return;
    if (track_.empty()) {
        status_ = kWin; // 轨道清空优先：压线一步恰好清空仍算胜
        return;
    }
    // 队列耗尽且轨道非空，或步数到达上限（LIMIT，0=不限）→ 负。
    // WIN 检查在前：上限最后一步恰好清空轨道判胜
    if (queue_.empty() || (limit_ > 0 && moves_ >= limit_))
        status_ = kLose;
    else
        status_ = kPlaying;
}

} // namespace zuma
