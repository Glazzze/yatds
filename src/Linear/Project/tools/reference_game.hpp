#ifndef ZUMA_TEST_REFERENCE_GAME_HPP
#define ZUMA_TEST_REFERENCE_GAME_HPP

// 测试专用参考引擎：顺序字符串、全量快照撤销、独立计分。
// 不调用 Game、Track 或生产代码的消除/撤销辅助函数。
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace zuma_test {

class ReferenceGame {
public:
    static constexpr std::int64_t cap = 9007199254740991LL;
    struct State {
        std::string track, queue;
        std::int64_t score = 0;
        int moves = 0;
        int status = 0; // 0 playing / 1 win / 2 lose，与协议含义对应
    };
    struct Event {
        int round, left, count;
        std::int64_t points;
        std::string balls, trackAfter;
    };

    ReferenceGame(std::string colors, std::string track, std::string queue, int limit)
        : colors_(std::move(colors)), limit_(limit)
    {
        state.track = std::move(track);
        state.queue = std::move(queue);
        // 初始化使用每次从头找第一段三连的全扫描，完全不依赖链表局部性。
        for (;;) {
            bool removed = false;
            for (std::size_t l = 0; l < state.track.size();) {
                std::size_t r = l + 1;
                while (r < state.track.size() && state.track[r] == state.track[l]) ++r;
                if (r - l >= 3) {
                    state.track.erase(l, r - l);
                    removed = true;
                    break;
                }
                l = r;
            }
            if (!removed) break;
        }
        refresh();
    }

    std::vector<Event> shoot(int pos, char color = 0)
    {
        requirePlaying();
        if (pos < 0 || pos > static_cast<int>(state.track.size()) || state.queue.empty())
            throw std::out_of_range("invalid shot");
        char original = state.queue[0];
        if (original == '?') {
            if (!color || colors_.find(color) == std::string::npos)
                throw std::invalid_argument("invalid color");
        } else if (color) {
            throw std::invalid_argument("unexpected color");
        }
        history_.push_back(state);
        state.queue.erase(0, 1);
        state.track.insert(pos, 1, original == '?' ? color : original);
        std::vector<Event> events;
        int unused = -1;
        if (original == '*') {
            // 炸弹的端点规则用区间夹紧计算，不模拟生产代码的扩展循环。
            const int count = std::min(3, static_cast<int>(state.track.size()));
            const int left = std::max(0, std::min(pos - 1,
                static_cast<int>(state.track.size()) - count));
            erase(left, count, 1, events, unused);
            settle(left, 2, events, unused);
        } else {
            settle(pos, 1, events, unused);
        }
        ++state.moves;
        refresh();
        return events;
    }

    void swap()
    {
        requirePlaying();
        if (state.queue.size() < 2) throw std::out_of_range("short queue");
        history_.push_back(state);
        std::swap(state.queue[0], state.queue[1]);
        ++state.moves;
        refresh();
    }

    std::vector<Event> reverse(int i, int j)
    {
        requirePlaying();
        if (i < 0 || j < i || j >= static_cast<int>(state.track.size()))
            throw std::out_of_range("invalid reverse");
        history_.push_back(state);
        std::reverse(state.track.begin() + i, state.track.begin() + j + 1);
        std::vector<Event> events;
        // 第二拼缝的坐标通过每次删除的区间作平移，完全不用节点指针存活判定。
        int second = j;
        settle(i, 1, events, second);
        if (second >= 0) {
            int unused = -1;
            settle(second, 1, events, unused);
        }
        ++state.moves;
        refresh();
        return events;
    }

    int hint() const
    {
        if (state.status != 0 || state.queue.empty() || state.queue[0] == '?') return -1;
        int best = -1, count = 0;
        // 穷举每个位置的首轮，独立模拟插入后的连续段。
        for (int pos = 0; pos <= static_cast<int>(state.track.size()); ++pos) {
            std::string s = state.track;
            s.insert(pos, 1, state.queue[0]);
            int c;
            if (state.queue[0] == '*') c = std::min(3, static_cast<int>(s.size()));
            else {
                int l = pos, r = pos + 1;
                while (l > 0 && s[l - 1] == s[pos]) --l;
                while (r < static_cast<int>(s.size()) && s[r] == s[pos]) ++r;
                c = r - l >= 3 ? r - l : 0;
            }
            if (c > count) { count = c; best = pos; }
        }
        return best;
    }

    bool undo()
    {
        if (history_.empty()) return false;
        state = history_.back();
        history_.pop_back();
        return true;
    }

    bool undoAvailable() const { return !history_.empty(); }
    State state;

private:
    void requirePlaying() const
    {
        if (state.status != 0) throw std::logic_error("terminal");
    }

    void refresh()
    {
        state.status = state.track.empty() ? 1 :
            (state.queue.empty() || (limit_ > 0 && state.moves >= limit_)) ? 2 : 0;
    }

    void erase(int left, int count, int round, std::vector<Event> &events, int &marker)
    {
        std::int64_t points = count * 10;
        for (int k = 1; k < round; ++k)
            points = points > cap / 2 ? cap : points * 2;
        const std::string balls = state.track.substr(left, count);
        state.track.erase(left, count);
        if (marker >= left && marker < left + count) marker = -1;
        else if (marker >= left + count) marker -= count;
        state.score = points >= cap - state.score ? cap : state.score + points;
        events.push_back({round, left, count, points, balls, state.track});
    }

    void settle(int pos, int round, std::vector<Event> &events, int &marker)
    {
        while (pos >= 0 && pos < static_cast<int>(state.track.size())) {
            int l = pos, r = pos + 1;
            const char c = state.track[pos];
            while (l > 0 && state.track[l - 1] == c) --l;
            while (r < static_cast<int>(state.track.size()) && state.track[r] == c) ++r;
            if (r - l < 3) break;
            erase(l, r - l, round++, events, marker);
            pos = l;
        }
    }

    std::string colors_;
    int limit_;
    std::vector<State> history_;
};

} // namespace zuma_test

#endif
