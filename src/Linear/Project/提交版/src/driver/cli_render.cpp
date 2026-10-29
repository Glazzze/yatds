#include "cli_render.hpp"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>

namespace zuma {
namespace {

/* ---------- ANSI 调色板 ---------- */

constexpr const char *kReset = "\x1b[0m";
constexpr const char *kCyan = "\x1b[1;36m";
constexpr const char *kYellow = "\x1b[1;33m";
constexpr const char *kDim = "\x1b[2m";

/* 8 色基础调色板（加粗），供助记字母与回退配色共用。 */
const char *paletteAt(int slot)
{
    static const char *kPalette[8] = {
        "\x1b[1;31m", // 红
        "\x1b[1;32m", // 绿
        "\x1b[1;34m", // 蓝
        "\x1b[1;33m", // 黄
        "\x1b[1;35m", // 洋红
        "\x1b[1;36m", // 青
        "\x1b[1;37m", // 白
        "\x1b[1;90m", // 灰
    };
    return kPalette[slot % 8];
}

/* 助记字母 → 调色板槽位；-1 = 无固定助记色（按声明顺序回退）。 */
int mnemonicSlot(char c)
{
    switch (std::toupper(static_cast<unsigned char>(c))) {
    case 'R': return 0; // red
    case 'G': return 1; // green
    case 'B': return 2; // blue
    case 'Y': return 3; // yellow
    case 'M': return 4; // magenta
    case 'C': return 5; // cyan
    case 'W': return 6; // white
    case 'K': return 7; // black/gray
    default: return -1;
    }
}

/* NO_COLOR 规范：变量存在且非空即视为关闭颜色。 */
bool envNonEmpty(const char *name)
{
    const char *v = std::getenv(name);
    return v != nullptr && *v != '\0';
}

void printUsage(std::ostream &os)
{
    os << "用法: zuma [--ui|--plain] [--color|--no-color] < inputs/caseXX.txt\n"
          "  从 stdin 读入局面与指令，按 docs/PROTOCOL.md 输出到 stdout。\n"
          "\n"
          "  --ui        打印交互提示（状态摘要、事件说明、错误建议）\n"
          "  --plain     仅输出协议内容（测试/提交通道；非终端时默认）\n"
          "  --color     强制使用 ANSI 颜色（优先于 NO_COLOR）\n"
          "  --no-color  强制关闭颜色\n"
          "  -h, --help  显示本帮助\n"
          "\n"
          "环境变量 NO_COLOR=1 可关闭颜色；输出重定向时默认即为纯文本。\n";
}

} // namespace

/* ---------- 命令行开关 ---------- */

bool resolveRenderOptions(int argc, char **argv, RenderOptions &out,
                          int &exitCode)
{
    bool ui = false, uiSet = false;
    bool color = false, colorSet = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--ui") {
            ui = true;
            uiSet = true;
        } else if (arg == "--plain") {
            ui = false;
            uiSet = true;
        } else if (arg == "--color") {
            color = true;
            colorSet = true;
        } else if (arg == "--no-color") {
            color = false;
            colorSet = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(std::cout);
            exitCode = 0;
            return false;
        } else {
            std::cerr << "未知参数: " << arg << "\n\n";
            printUsage(std::cerr);
            exitCode = 2;
            return false;
        }
    }

    const bool tty = ::isatty(STDOUT_FILENO) != 0;
    if (!uiSet)
        ui = tty;

    if (!colorSet) {
        const char *term = std::getenv("TERM");
        const bool dumb = term != nullptr && std::strcmp(term, "dumb") == 0;
        // 默认只在"终端 + 交互模式"下上色：强制 --ui 但重定向时不污染文件
        color = tty && ui && !envNonEmpty("NO_COLOR") && !dumb;
    }

    out.interactive = ui;
    out.color = color;
    return true;
}

/* ---------- Renderer ---------- */

Renderer::Renderer(RenderOptions opt, std::string colors)
    : opt_(opt), colors_(std::move(colors))
{
    // 配色：助记字母用固定色；其余字母按 COLORS 声明顺序取未被占用的
    // 调色板槽位（保证同一局面内不同颜色尽量可区分）。两趟：先占位
    // 助记色，再按声明顺序回退，结果与 colors_ 下标一一对应。
    bool used[8] = {false, false, false, false, false, false, false, false};
    for (char c : colors_) {
        const int slot = mnemonicSlot(c);
        if (slot >= 0)
            used[slot] = true;
    }
    for (char c : colors_) {
        const int mslot = mnemonicSlot(c);
        if (mslot >= 0) {
            ballCodes_.push_back(paletteAt(mslot));
            continue;
        }
        int slot = 0;
        while (slot < 8 && used[slot])
            ++slot;
        if (slot < 8) {
            used[slot] = true;
            ballCodes_.push_back(paletteAt(slot));
        } else {
            ballCodes_.push_back(
                paletteAt(static_cast<int>(ballCodes_.size() % 8)));
        }
    }
}

std::string Renderer::paint(const std::string &text, const char *code) const
{
    if (!opt_.color || text.empty())
        return text;
    return std::string(code) + text + kReset;
}

std::string Renderer::paintBalls(const std::string &balls) const
{
    if (!opt_.color || balls.empty())
        return balls;

    std::string out;
    out.reserve(balls.size() * 12);
    for (char c : balls) {
        const char *code = nullptr;
        if (c == kBomb) {
            code = "\x1b[1;97;41m"; // 白字红底
        } else if (c == kChameleon) {
            code = "\x1b[1;95m"; // 亮洋红
        } else {
            const std::size_t idx = colors_.find(c);
            if (idx != std::string::npos && idx < ballCodes_.size())
                code = ballCodes_[idx];
        }
        if (!code) {
            out.push_back(c);
            continue;
        }
        out += code;
        out.push_back(c);
        out += kReset;
    }
    return out;
}

void Renderer::printBanner(std::ostream &os) const
{
    if (!opt_.interactive)
        return;
    os << paint("═══ 祖玛 · CLI 模式 ═══", kCyan) << '\n'
       << paint("  帮助: HELP · 退出: QUIT · 关闭颜色: NO_COLOR=1", kDim)
       << '\n';
}

void Renderer::printEvent(std::ostream &os, const std::string &text) const
{
    if (!opt_.interactive)
        return;
    os << "  " << paint("✦", kYellow) << ' ' << text << '\n';
}

void Renderer::printElimNotice(std::ostream &os, const Game::ElimEvent &ev) const
{
    if (!opt_.interactive)
        return;

    std::string text;
    if (ev.round <= 1) {
        if (ev.balls.find(kBomb) != std::string::npos)
            text = "炸弹生效：以炸弹为中心消除 " + std::to_string(ev.count)
                   + " 球，+" + std::to_string(ev.points) + " 分";
        else
            text = "消除 " + std::to_string(ev.count) + " 球，+"
                   + std::to_string(ev.points) + " 分";
    } else {
        text = "连锁 ×" + std::to_string(ev.round) + "：再消除 "
               + std::to_string(ev.count) + " 球，+"
               + std::to_string(ev.points) + " 分";
    }
    printEvent(os, text);
}

void Renderer::printStatus(std::ostream &os, const Game &game) const
{
    if (!opt_.interactive)
        return;
    os << "  " << paint("·", kDim) << " 分数 " << game.score() << " · 步数 "
       << game.moves() << " · 轨道 " << game.track().size() << " 球"
       << " · 队列 " << game.queue().size() << " 球";
    if (game.limit() > 0)
        os << " · 上限 " << game.limit();
    os << '\n';
}

void Renderer::printHintNotice(std::ostream &os, const Game &game, int pos) const
{
    if (!opt_.interactive)
        return;

    if (pos >= 0) {
        printEvent(os, "提示：推荐在第 " + std::to_string(pos)
                           + " 位发射（可立即消除）。");
        return;
    }
    const std::string q = game.queueString();
    if (game.status() != Game::kPlaying)
        printEvent(os, "提示：对局已结束，可用 UNDO 回退或 QUIT 退出。");
    else if (q.empty())
        printEvent(os, "提示：待发射队列已空，无可发射的球。");
    else if (q[0] == kChameleon)
        printEvent(os, "提示：队首是变色球 ?，需先确定颜色，暂不提示。");
    else
        printEvent(os, "提示：当前没有可立即消除的位置。");
}

void Renderer::printErrorHint(std::ostream &os, const Command &cmd,
                              const Game &game, const std::string &reason) const
{
    if (!opt_.interactive)
        return;

    const std::size_t n = game.track().size();
    std::string hint;

    if (reason == "发射位置越界") {
        hint = "位置越界——轨道共 " + std::to_string(n) + " 球，可插入位置为 0.."
               + std::to_string(n) + "（0 为最左端，" + std::to_string(n)
               + " 为尾插）。";
    } else if (reason == "待发射队列为空") {
        hint = "待发射队列已空，没有可发射的球。";
    } else if (reason == "变色球必须指定 COLORS 内的颜色") {
        hint = "队首是变色球 ?，必须指定颜色：SHOOT <位置> <颜色>，颜色取自 COLORS "
               + colors_ + "。";
    } else if (reason == "仅变色球可指定颜色") {
        hint = "只有变色球 ? 需要颜色参数；普通球直接写 SHOOT <位置>。";
    } else if (reason == "反转区间越界") {
        hint = "区间非法——需满足 0 <= i <= j < 轨道长度 " + std::to_string(n)
               + "。";
    } else if (reason == "队列不足两球，无法交换") {
        hint = "SWAP 需要队列中至少 2 个待发射球。";
    } else if (reason == "撤销栈为空") {
        hint = "当前没有可撤销的操作（HINT 与非法指令不计步）。";
    } else if (reason == "终局后不接受指令" || reason == "终局态不可发射"
               || reason == "终局态不可反转") {
        hint = "对局已结束——可用 UNDO 撤销终局步继续，或 QUIT 退出。";
    } else if (cmd.kind == Command::kCmdInvalid) {
        // 语法错误：按指令给用法提示
        if (cmd.reason.rfind("SHOOT", 0) == 0)
            hint = "用法：SHOOT <位置> [颜色]，位置为 0 起的整数。";
        else if (cmd.reason.rfind("REVERSE", 0) == 0)
            hint = "用法：REVERSE <i> <j>，反转为闭区间。";
        else if (cmd.reason.rfind("HELP", 0) == 0)
            hint = "HELP 不接受参数，直接输入 HELP 即可。";
        else
            hint = "输入 HELP 查看全部指令。";
    }

    if (!hint.empty())
        os << "  " << paint("▸ 提示：", kYellow) << hint << '\n';
}

void Renderer::printHelp(std::ostream &os) const
{
    const std::size_t kNameWidth = 24; // 命令列宽（与交互/纯文本一致）
    auto row = [&](const std::string &name, const std::string &desc) {
        const std::size_t pad = name.size() < kNameWidth
                                    ? kNameWidth - name.size()
                                    : 1;
        os << "  " << paint(name, kCyan) << std::string(pad, ' ') << desc
           << '\n';
    };

    os << "可用指令：\n";
    row("SHOOT <pos> [color]", "发射队首球到下标 pos（0 起，p=n 为尾插）");
    row("UNDO", "撤销上一步（含连锁回滚，可撤销终局步）");
    row("SWAP", "交换待发射队列的头两球");
    row("HINT", "提示队首球的最优插入位置（-1 = 无可消除位）");
    row("REVERSE <i> <j>", "反转轨道闭区间 [i, j]（0 <= i <= j < n）");
    row("HELP", "显示本帮助（不计步数）");
    row("QUIT", "退出并输出终局");
    os << "特殊球：* 炸弹（消除邻域至多 3 球）  ? 变色（SHOOT 时须指定颜色）\n";
    os << "位置：0-based，从轨道左端起；SHOOT <pos> 表示插入后新球位于下标 pos。\n";
}

} // namespace zuma