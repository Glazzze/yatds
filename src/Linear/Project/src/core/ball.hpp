#ifndef ZUMA_CORE_BALL_HPP
#define ZUMA_CORE_BALL_HPP

namespace zuma {

/*
 * 彩球：普通球用大写字母（'R'/'G'/'B'/'Y'…，须 ∈ COLORS 声明），
 * 特殊球保留两个符号（仅允许出现在待发射队列中，见 PROTOCOL.md §2）。
 */
using Ball = char;

constexpr Ball kBomb = '*';        // 炸弹球：发射后消除插入点邻域，无视颜色
constexpr Ball kChameleon = '?';   // 变色球：发射时必须指定目标颜色

} // namespace zuma

#endif // ZUMA_CORE_BALL_HPP
