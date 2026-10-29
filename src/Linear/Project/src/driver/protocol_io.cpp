#include "protocol_io.hpp"

#include <cctype>
#include <sstream>
#include <vector>

namespace zuma {

/* finishSetup 的前置声明（定义在下方，readHeader 中提前使用） */
static Setup finishSetup(Setup su);

/* ---------- 工具：分词与修剪 ---------- */

static std::string trim(const std::string &s)
{
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b])))
        ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
        --e;
    return s.substr(b, e - b);
}

static std::vector<std::string> split(const std::string &s)
{
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok)
        out.push_back(tok);
    return out;
}

/* 解析非负整数（十进制，全长有效才合法）。失败返回 false。 */
static bool parseInt(const std::string &tok, int &value)
{
    if (tok.empty())
        return false;
    long v = 0;
    for (char c : tok) {
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return false;
        v = v * 10 + (c - '0');
        if (v > 100000000) // 上限保护（轨道/队列规模远小于此）
            return false;
    }
    value = static_cast<int>(v);
    return true;
}

/* ---------- 指令行解析（PROTOCOL.md §1） ---------- */

Command parseCommand(const std::string &lineIn)
{
    Command cmd;
    cmd.raw = trim(lineIn);

    // 空行与 # 注释：静默跳过（不回显、不计步）
    if (cmd.raw.empty() || cmd.raw[0] == '#') {
        cmd.kind = Command::kCmdComment;
        return cmd;
    }

    std::vector<std::string> tok = split(cmd.raw);
    // 指令名不区分大小写（协议 v1.1）：仅规范化首个 token 用于匹配，
    // 回显仍用原始行（cmd.raw）。
    std::string head = tok[0];
    for (char &c : head)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    if (head == "SHOOT") {
        cmd.kind = Command::kCmdShoot;
        if (tok.size() < 2) {
            cmd.kind = Command::kCmdInvalid;
            cmd.reason = "SHOOT 缺少位置参数";
        } else if (!parseInt(tok[1], cmd.pos)) {
            cmd.kind = Command::kCmdInvalid;
            cmd.reason = "SHOOT 位置不是非负整数";
        } else if (tok.size() == 2) {
            cmd.hasColor = false;
        } else if (tok.size() == 3 && tok[2].size() == 1
                   && std::isupper(static_cast<unsigned char>(tok[2][0]))) {
            cmd.color = tok[2][0];
            cmd.hasColor = true;
        } else {
            cmd.kind = Command::kCmdInvalid;
            cmd.reason = "SHOOT 参数格式错误（<pos> 或 <pos> <大写字母>）";
        }
    } else if (head == "REVERSE") {
        cmd.kind = Command::kCmdReverse;
        if (tok.size() != 3) {
            cmd.kind = Command::kCmdInvalid;
            cmd.reason = "REVERSE 需要 <i> <j> 两个参数";
        } else if (!parseInt(tok[1], cmd.pos) || !parseInt(tok[2], cmd.end)) {
            cmd.kind = Command::kCmdInvalid;
            cmd.reason = "REVERSE 参数不是非负整数";
        }
    } else if (head == "UNDO" && tok.size() == 1) {
        cmd.kind = Command::kCmdUndo;
    } else if (head == "SWAP" && tok.size() == 1) {
        cmd.kind = Command::kCmdSwap;
    } else if (head == "HINT" && tok.size() == 1) {
        cmd.kind = Command::kCmdHint;
    } else if (head == "HELP" && tok.size() == 1) {
        cmd.kind = Command::kCmdHelp;
    } else if (head == "HELP") {
        cmd.kind = Command::kCmdInvalid;
        cmd.reason = "HELP 不接受参数";
    } else if (head == "QUIT" && tok.size() == 1) {
        cmd.kind = Command::kCmdQuit;
    } else {
        cmd.kind = Command::kCmdInvalid;
        cmd.reason = "未知指令或参数数量错误";
    }
    return cmd;
}

/* ---------- 头部解析（PROTOCOL.md §1） ---------- */

Setup readHeader(std::istream &in)
{
    Setup su;

    enum { kColors, kTrack, kQueue, kLimitOrReady, kDone } phase = kColors;
    std::string line;
    while (phase != kDone && std::getline(in, line)) {
        std::string t = trim(line);
        if (t.empty() || t[0] == '#')
            continue; // 头部前后的空行/注释忽略

        if (phase == kColors) {
            if (t.rfind("COLORS ", 0) == 0) {
                su.colors = trim(t.substr(7));
            } else if (t == "COLORS") {
                su.colors = "";
                // "COLORS 空串" 无意义：颜色集合不可为空
                su.error = "COLORS 行缺少颜色集合";
                su.ok = false;
                return su;
            } else {
                su.error = "首行必须是 COLORS 行";
                su.ok = false;
                return su;
            }
            phase = kTrack;
        } else if (phase == kTrack) {
            if (t.rfind("TRACK ", 0) == 0)
                su.track = trim(t.substr(6));
            else if (t == "TRACK")
                su.track = ""; // 空轨道合法
            else {
                su.error = "第二行必须是 TRACK 行";
                su.ok = false;
                return su;
            }
            phase = kQueue;
        } else if (phase == kQueue) {
            if (t.rfind("QUEUE ", 0) == 0)
                su.queue = trim(t.substr(6));
            else if (t == "QUEUE")
                su.queue = ""; // 空队列合法
            else {
                su.error = "第三行必须是 QUEUE 行";
                su.ok = false;
                return su;
            }
            phase = kLimitOrReady;
        } else { // kLimitOrReady
            if (t.rfind("LIMIT ", 0) == 0) {
                if (!parseInt(trim(t.substr(6)), su.limit) || su.limit <= 0) {
                    su.error = "LIMIT 行参数不是正整数";
                    su.ok = false;
                    return su;
                }
                su.hasLimit = true;
                // LIMIT 之后继续等 READY 或首个指令行
            } else if (t == "READY") {
                phase = kDone;
            } else {
                // 首个指令行（无 READY 的情况）：getline 已消费该行，
                // 无法回退。经 error 字段传回，主循环先于读流处理它。
                su.error = t;
                phase = kDone;
                return finishSetup(su);
            }
        }
    }
    // EOF 退出循环：三行头已齐全（phase >= kLimitOrReady）时视为合法
    // —— 无 READY、无指令，主循环将走 EOF-QUIT 路径（协议 §7）。
    // 头部行没读全才是真不完整。
    if (phase < kLimitOrReady) {
        su.error = "头部不完整（缺 COLORS/TRACK/QUEUE 行或提前 EOF）";
        su.ok = false;
        return su;
    }
    return finishSetup(su);
}

/* 头部公共校验：颜色集合非空、全大写、无重复；轨道只含 COLORS 内
 * 字母（禁特殊球）；队列额外允许 * 与 ?。 */
static Setup finishSetup(Setup su)
{
    if (su.colors.empty()) {
        su.ok = false;
        su.error = "颜色集合为空";
        return su;
    }
    if (su.colors.size() < 2 || su.colors.size() > 8) {
        su.ok = false;
        su.error = "颜色数量必须为 2–8 种";
        return su;
    }
    if (su.track.size() > Game::kMaxTrack || su.queue.size() > Game::kMaxQueue) {
        su.ok = false;
        su.error = "初始轨道最多 1000 球，队列最多 2000 球";
        return su;
    }
    for (char c : su.colors) {
        if (!std::isupper(static_cast<unsigned char>(c))) {
            su.ok = false;
            su.error = "颜色集合含非大写字母";
            return su;
        }
    }
    for (std::size_t i = 0; i < su.colors.size(); ++i)
        for (std::size_t j = i + 1; j < su.colors.size(); ++j)
            if (su.colors[i] == su.colors[j]) {
                su.ok = false;
                su.error = "颜色集合含重复字母";
                return su;
            }
    for (char c : su.track) {
        if (su.colors.find(c) == std::string::npos) {
            su.ok = false;
            su.error = "轨道含非法球符号";
            return su;
        }
    }
    for (char c : su.queue) {
        if (su.colors.find(c) == std::string::npos && c != kBomb
            && c != kChameleon) {
            su.ok = false;
            su.error = "待发射队列含非法球符号";
            return su;
        }
    }
    su.ok = true;
    return su;
}

/* ---------- 事件输出（PROTOCOL.md §3.2/§3.3 纯文本形态） ---------- */

void printElim(std::ostream &os, const Game::ElimEvent &ev,
               const std::string &trackAfter, const BallPainter &paint)
{
    const std::string balls = paint ? paint(ev.balls) : ev.balls;
    const std::string track = paint ? paint(trackAfter) : trackAfter;
    os << "ELIM round=" << ev.round << " left=" << ev.left
       << " count=" << ev.count << " balls=" << balls
       << " score=+" << ev.points << '\n';
    os << "TRACK " << track << '\n';
}

void printFinal(std::ostream &os, Game::Status st, std::int64_t score,
                int moves)
{
    switch (st) {
    case Game::kWin:
        os << "WIN SCORE=" << score << " MOVES=" << moves << '\n';
        break;
    case Game::kLose:
        os << "LOSE SCORE=" << score << " MOVES=" << moves << '\n';
        break;
    case Game::kQuit:
        os << "QUIT_SCORE SCORE=" << score << " MOVES=" << moves << '\n';
        break;
    default:
        break; // kPlaying 不输出终局
    }
}

} // namespace zuma
