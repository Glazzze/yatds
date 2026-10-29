#ifndef ZUMA_DRIVER_PROTOCOL_IO_HPP
#define ZUMA_DRIVER_PROTOCOL_IO_HPP

#include "../core/game.hpp"

#include <functional>
#include <iosfwd>
#include <string>

namespace zuma {

/*
 * 指令行解析结果（driver 主循环的数据单元）。
 * 每条输入行解析成一个 Command；解析失败（语法错误）也产出 Command，
 * kind = kCmdInvalid 携带原因文本 —— 统一走"回显 + ERROR"路径。
 */
struct Command {
    enum Kind {
        kCmdShoot,     // SHOOT <pos> [color]
        kCmdUndo,      // UNDO
        kCmdSwap,      // SWAP
        kCmdHint,      // HINT
        kCmdReverse,   // REVERSE <i> <j>
        kCmdHelp,      // HELP（协议 v1.1，元指令：不计步、终局后仍可用）
        kCmdQuit,      // QUIT
        kCmdInvalid,   // 语法错误（reason 有效）
        kCmdComment,   // 空行 / # 注释（跳过，不回显）
    };

    Kind kind = kCmdComment;
    int pos = 0;           // SHOOT：插入下标；REVERSE：区间左端
    int end = 0;           // REVERSE：区间右端
    char color = 0;        // SHOOT：变色球定色（0 = 未携带）
    bool hasColor = false; // SHOOT 是否携带 <color>
    std::string reason;    // kCmdInvalid 时的错误原因（ERROR 行用）
    std::string raw;       // 原始行（回显用，已去除首尾空白）
};

/* 解析一条指令行（已去除换行符）。空行/注释 → kCmdComment。
 * 指令名不区分大小写（PROTOCOL.md v1.1）；参数保持原样。 */
Command parseCommand(const std::string &line);

/* ---------- 头部（局面装载）解析 ---------- */

struct Setup {
    std::string colors; // COLORS 行内容（空 = 行缺失，报错）
    std::string track;  // TRACK 行内容（可为空串）
    std::string queue;  // QUEUE 行内容（可为空串）
    int limit = 0;      // LIMIT 行（0 = 无）
    bool hasLimit = false;
    bool ok = false;    // 解析是否成功
    std::string error;  // 失败原因（输出到 stderr 后退出）
};

/*
 * 从 in 读头部：COLORS / TRACK / QUEUE 行（顺序固定，TRACK/QUEUE 内容
 * 可空），可选 LIMIT 行，直到 READY 或首个指令行。读到首个指令行时
 * 回退流位置（peek/unget），由主循环继续读。
 */
Setup readHeader(std::istream &in);

/* ---------- 事件输出（PROTOCOL.md §3.2，纯文本基准形态） ---------- */

/* 球串渲染钩子（阶段 3 · T3.2 的 ANSI 上色）；空钩子 = 原样输出。 */
using BallPainter = std::function<std::string(const std::string &)>;

/*
 * 输出一轮消除事件行 + 消除后轨道行：
 *   ELIM round=<r> left=<l> count=<c> balls=<b> score=+<p>
 *   TRACK <轨道>
 * paint 非空时分别作用于 balls 与 trackAfter（颜色只影响终端显示，
 * 纯文本模式下内容逐字节不变）。
 */
void printElim(std::ostream &os, const Game::ElimEvent &ev,
               const std::string &trackAfter,
               const BallPainter &paint = BallPainter());

/*
 * 终局行（PROTOCOL.md §3.3）：
 *   WIN SCORE=<s> MOVES=<m> / LOSE ... / QUIT_SCORE ...
 */
void printFinal(std::ostream &os, Game::Status st, std::int64_t score,
                int moves);

} // namespace zuma

#endif // ZUMA_DRIVER_PROTOCOL_IO_HPP
