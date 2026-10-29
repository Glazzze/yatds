#ifndef ZUMA_CORE_WAITING_QUEUE_HPP
#define ZUMA_CORE_WAITING_QUEUE_HPP

#include "ball.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace zuma {

/*
 * 待发射彩球队列 —— 环形缓冲队列（D3 选 C：本项目自实现，语义参照
 * Baseline CircularQueue：固定容量 + front/back 指针 + 满判断）。
 *
 * 与 Baseline 版的差异（为何不复用）：
 *   - take() 合并 front + pop，契合"取球即发射"语义；
 *   - swap_front_two()（SWAP 道具）与 unshift()（UNDO 放回队首）是
 *     祖玛专属操作，CircularQueue 的接口不适合为此开口子；
 *   - snapshot() 支持渲染导出，避免破坏性遍历。
 *
 * 规模上限（PROTOCOL.md §6）：容量 ≤ 2×10^3。
 */
template <typename T>
class RingQueue {
public:
    explicit RingQueue(std::size_t capacity)
        : buffer_(capacity), capacity_(capacity)
    {
        if (capacity == 0)
            throw std::invalid_argument("队列容量必须为正");
    }

    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == capacity_; }
    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }

    /* 入队（装填阶段用）。满队抛 overflow_error。 */
    void push(const T &value)
    {
        if (full())
            throw std::overflow_error("待发射队列已满");
        buffer_[back_] = value;
        back_ = (back_ + 1) % capacity_;
        ++size_;
    }

    /* 取队首并出队（发射）。空队抛 out_of_range。 */
    T take()
    {
        if (empty())
            throw std::out_of_range("待发射队列为空");
        T value = buffer_[front_];
        front_ = (front_ + 1) % capacity_;
        --size_;
        return value;
    }

    /* 放回队首（撤销 SHOOT 用，O(1) 逆推 front 下标）。满队抛 overflow_error。
     * 撤销语义下不会满：take 腾出的空位恰好被 unshift 占回。 */
    void unshift(const T &value)
    {
        if (full())
            throw std::overflow_error("队列已满，无法放回队首");
        front_ = (front_ + capacity_ - 1) % capacity_;
        buffer_[front_] = value;
        ++size_;
    }

    /* 只读访问队首（不出队）。空队抛 out_of_range。 */
    const T &front() const
    {
        if (empty())
            throw std::out_of_range("待发射队列为空");
        return buffer_[front_];
    }

    /* 只读访问次队首（SWAP 预览/提示用）。size < 2 抛 out_of_range。 */
    const T &second() const
    {
        if (size_ < 2)
            throw std::out_of_range("队列不足两球，无次队首");
        return buffer_[(front_ + 1) % capacity_];
    }

    /* 交换队首与次队首（SWAP 道具）。size < 2 抛 out_of_range。 */
    void swap_front_two()
    {
        if (size_ < 2)
            throw std::out_of_range("队列不足两球，无法交换");
        std::swap(buffer_[front_], buffer_[(front_ + 1) % capacity_]);
    }

    /* 队列快照：队首在前（渲染导出与撤销比对用，非破坏性）。 */
    std::vector<T> snapshot() const
    {
        std::vector<T> out;
        out.reserve(size_);
        for (std::size_t i = 0; i < size_; ++i)
            out.push_back(buffer_[(front_ + i) % capacity_]);
        return out;
    }

private:
    std::vector<T> buffer_; // 环形缓冲区
    std::size_t front_ = 0; // 队首下标
    std::size_t back_ = 0;  // 队尾下一空位下标
    std::size_t size_ = 0;
    std::size_t capacity_;  // 固定容量（构造后不变）
};

} // namespace zuma

#endif // ZUMA_CORE_WAITING_QUEUE_HPP
