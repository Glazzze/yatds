#ifndef ZUMA_DRIVER_CLI_RENDER_HPP
#define ZUMA_DRIVER_CLI_RENDER_HPP

#include "protocol_io.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace zuma {

/*
 * CLI 渲染开关（阶段 3 · T3.1–T3.2）。
 *
 * 两种输出模式共用同一份协议事件数据：
 *   - interactive=false：纯协议输出（PROTOCOL.md §3 的逐字节契约，
 *     test 比对与 outputs 目录下的标准输出以它为准）；
 *   - interactive=true：协议行 + 以两个空格开头的交互提示行（状态摘要、
 *     事件说明、错误建议），球串在 color=true 时逐球上 ANSI 颜色。
 * 交互行不属于输出契约，纯协议模式下一条都不会出现。
 */
struct RenderOptions {
    bool interactive = false; // 交互提示行
    bool color = false;       // ANSI 颜色码
};

/*
 * 解析命令行开关：
 *   --ui / --plain        强制交互渲染 / 纯协议输出
 *   --color / --no-color  强制 ANSI 上色 / 关闭
 *   -h, --help            打印用法后退出
 *
 * 默认值：交互 = stdout 是终端；颜色 = stdout 是终端 && 交互 && 未设
 * NO_COLOR && TERM!=dumb（强制 --ui 但重定向时不污染输出文件）。
 * 显式 --color 优先于 NO_COLOR（便于演示强制上色）。
 *
 * 返回 true 继续运行；false 表示应立即退出（--help 为 0，非法参数为 2），
 * 退出码写入 exitCode。
 */
bool resolveRenderOptions(int argc, char **argv, RenderOptions &out,
                          int &exitCode);

/*
 * 终端渲染器：球串上色 + 交互提示（T3.1 事件提示 / T3.2 颜色 / T3.3 友好错误）。
 * 只依赖 core 查询接口，不改游戏状态。
 */
class Renderer {
public:
    Renderer(RenderOptions opt, std::string colors);

    bool interactive() const { return opt_.interactive; }
    bool color() const { return opt_.color; }

    /* 球串逐球上色（color=false 原样返回）。普通球按 COLORS 映射，
     * 特殊球 * / ? 有固定样式。 */
    std::string paintBalls(const std::string &balls) const;

    /* 开局横幅（仅交互模式）。 */
    void printBanner(std::ostream &os) const;

    /* 交互提示行（仅交互模式）：以 "  ✦ " 前缀。 */
    void printEvent(std::ostream &os, const std::string &text) const;

    /* 每轮消除的人类可读说明（消除/连锁/炸弹，仅交互模式）。 */
    void printElimNotice(std::ostream &os, const Game::ElimEvent &ev) const;

    /* 状态摘要（仅交互模式）：分数/步数/轨道与队列剩余球数。 */
    void printStatus(std::ostream &os, const Game &game) const;

    /* HINT 结果的解释（仅交互模式），pos<0 时区分空队列/变色球/无可消位。 */
    void printHintNotice(std::ostream &os, const Game &game, int pos) const;

    /* 错误后的友好建议（仅交互模式）。协议 ERROR 行保持不变，本方法只
     * 追加一行 "  ▸ 提示：..."；无法归类的错误不输出。 */
    void printErrorHint(std::ostream &os, const Command &cmd,
                        const Game &game, const std::string &reason) const;

    /* HELP 指令正文（两种模式均输出纯文本；交互模式下命令名上色）。 */
    void printHelp(std::ostream &os) const;

private:
    /* 文本上色（color=false 或空串原样返回）。 */
    std::string paint(const std::string &text, const char *code) const;

    RenderOptions opt_;
    std::string colors_;                 // COLORS 集合（决定普通球配色）
    std::vector<const char *> ballCodes_; // 与 colors_ 下标对齐的 ANSI 码
};

} // namespace zuma

#endif // ZUMA_DRIVER_CLI_RENDER_HPP