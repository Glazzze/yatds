/*
 * 祖玛游戏 · 主程序入口（阶段 2 · T2.6 指令 driver；阶段 3 · T3.1–T3.3 渲染）
 *
 * 按 docs/PROTOCOL.md v1.1 执行：stdin 读头部与指令 → 调 core Game →
 * stdout 输出协议格式。core 层零 IO（分层见 docs/design.md §1）。
 *
 * 输出双模式（D1 + 阶段 3）：
 *   - 纯协议模式（默认：stdout 非终端；或 --plain）：逐字节契约输出，
 *     test / outputs 比对以它为准；
 *   - 交互模式（默认：stdout 是终端；或 --ui）：协议行 + 以两个空格开头
 *     的状态摘要/事件说明/错误建议；--color 时球串上 ANSI 颜色，
 *     NO_COLOR=1 可关闭。交互行不属于协议，纯协议模式不输出。
 *
 * 用法：./zuma < inputs/caseXX.txt          （纯协议，测试通道）
 *       ./zuma --ui < inputs/caseXX.txt     （强制交互渲染）
 */
#include "core/game.hpp"
#include "driver/cli_render.hpp"
#include "driver/protocol_io.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char **argv)
{
    std::ios::sync_with_stdio(false);

    /* ---------- 0. CLI 开关（T3.1–T3.2） ---------- */

    zuma::RenderOptions ropt;
    int exitCode = 0;
    if (!zuma::resolveRenderOptions(argc, argv, ropt, exitCode))
        return exitCode;

    /* ---------- 1. 头部：解析 + 回显 ---------- */

    zuma::Setup su = zuma::readHeader(std::cin);
    if (!su.ok) {
        // su.ok=true 且 error 非空 = "error 传回首个指令行"路径
        std::cerr << "输入错误: " << su.error << '\n';
        return 1;
    }

    zuma::Renderer render(ropt, su.colors);
    render.printBanner(std::cout);

    std::cout << "COLORS " << su.colors << '\n'
              << "TRACK " << render.paintBalls(su.track) << '\n'
              << "QUEUE " << render.paintBalls(su.queue) << '\n';
    if (su.hasLimit)
        std::cout << "LIMIT " << su.limit << '\n';
    std::cout << "READY\n";

    /* ---------- 2. 装载 Game（core 构造含装载 settle） ---------- */

    zuma::Game game(su.colors, su.track, su.queue, su.hasLimit ? su.limit : 0);

    // 装载即终局（如空轨道直接 WIN、README 示例连锁归空）：输出终局行
    if (game.status() != zuma::Game::kPlaying) {
        zuma::printFinal(std::cout, game.status(), game.score(), game.moves());
    }
    render.printStatus(std::cout, game); // 交互模式：初始局面摘要

    /* ---------- 3. 主循环：逐行指令 ---------- */

    // readHeader 可能把首个指令行放在 su.error 里传回（无 READY 时）
    std::string pendingLine;
    bool hasPending = false;
    if (!su.error.empty()) {
        pendingLine = su.error;
        hasPending = true;
    }

    // 球串上色钩子（纯协议模式下原样返回）
    auto paint = [&render](const std::string &s) {
        return render.paintBalls(s);
    };

    bool quitDone = false;   // QUIT 指令输出终局后立即结束读取
    bool finalShown = game.status() != zuma::Game::kPlaying; // 装载终局亦只打印一次
    std::string line;
    while (true) {
        if (hasPending) {
            line = pendingLine;
            hasPending = false;
        } else {
            if (!std::getline(std::cin, line))
                break; // EOF：视同 QUIT（PROTOCOL.md §7）
        }

        zuma::Command cmd = zuma::parseCommand(line);
        if (cmd.kind == zuma::Command::kCmdComment)
            continue; // 空行/注释：静默跳过

        // 回显指令行。例外：QUIT 只输出终局；HINT 的回显与结果行合并
        // （协议 §3.2：HINT 行即结果行，避免 HINT HINT 0 双行）
        if (cmd.kind == zuma::Command::kCmdQuit) {
            game.quit();
            render.printEvent(std::cout, "已退出对局。");
            zuma::printFinal(std::cout, game.status(), game.score(),
                             game.moves());
            quitDone = true;
            break;
        }
        if (cmd.kind != zuma::Command::kCmdHint)
            std::cout << cmd.raw << '\n';

        // 终局后只有 UNDO（回退）与 HELP（元指令）被执行；其余指令
        // 回显后报错（协议 v1.1 §3.3）。
        if (game.status() != zuma::Game::kPlaying
            && cmd.kind != zuma::Command::kCmdUndo
            && cmd.kind != zuma::Command::kCmdHelp) {
            // 合法 HINT 的结果行回显例外仅适用于 PLAYING；被拒绝时仍回显原文。
            if (cmd.kind == zuma::Command::kCmdHint)
                std::cout << cmd.raw << '\n';
            std::cout << "ERROR 终局后不接受指令\n";
            render.printErrorHint(std::cout, cmd, game, "终局后不接受指令");
            continue;
        }

        switch (cmd.kind) {
        case zuma::Command::kCmdShoot: {
            try {
                const int colorArg = cmd.hasColor ? cmd.color : 0;
                auto events = game.shoot(cmd.pos, colorArg);
                if (cmd.hasColor)
                    render.printEvent(std::cout,
                                      std::string("变色球落定为 ")
                                          + cmd.color + "。");
                for (const auto &ev : events) {
                    zuma::printElim(std::cout, ev, ev.trackAfter, paint);
                    render.printElimNotice(std::cout, ev);
                }
                // 消除事件已各带一条 TRACK；无消除时补一条（协议 §3.2：
                // 每条 SHOOT 至少一条 TRACK + 一条 QUEUE）
                if (events.empty()) {
                    std::cout << "TRACK " << paint(game.trackString()) << '\n';
                    render.printEvent(std::cout, "本次发射未触发消除。");
                }
                std::cout << "QUEUE " << paint(game.queueString()) << '\n';
                render.printStatus(std::cout, game);
            } catch (const std::exception &e) {
                // core 抛出的 what() 已是协议原因短语
                std::cout << "ERROR " << e.what() << '\n';
                render.printErrorHint(std::cout, cmd, game, e.what());
            }
            break;
        }
        case zuma::Command::kCmdReverse: {
            try {
                auto events = game.reverse(cmd.pos, cmd.end);
                render.printEvent(std::cout,
                                  "道具 REVERSE 生效：反转区间 ["
                                      + std::to_string(cmd.pos) + ", "
                                      + std::to_string(cmd.end) + "]。");
                for (const auto &ev : events) {
                    zuma::printElim(std::cout, ev, ev.trackAfter, paint);
                    render.printElimNotice(std::cout, ev);
                }
                // 消除事件已各带一条 TRACK；无消除时补一条（消除后
                // 最后一条 ELIM 的 TRACK 已是终态）
                if (events.empty()) {
                    std::cout << "TRACK " << paint(game.trackString()) << '\n';
                    render.printEvent(std::cout, "反转后未触发消除。");
                }
                render.printStatus(std::cout, game);
            } catch (const std::exception &e) {
                std::cout << "ERROR " << e.what() << '\n';
                render.printErrorHint(std::cout, cmd, game, e.what());
            }
            break;
        }
        case zuma::Command::kCmdUndo: {
            if (!game.undo()) {
                std::cout << "ERROR 撤销栈为空\n";
                render.printErrorHint(std::cout, cmd, game, "撤销栈为空");
            } else {
                std::cout << "TRACK " << paint(game.trackString()) << '\n';
                std::cout << "QUEUE " << paint(game.queueString()) << '\n';
                render.printEvent(std::cout,
                                  "已撤销上一步，分数与步数同步回滚。");
                render.printStatus(std::cout, game);
                // 从终局回退：终局标记复位，游戏继续（后续再终局
                // 时会重新打印终局行）
                finalShown = false;
            }
            break;
        }
        case zuma::Command::kCmdSwap: {
            try {
                game.swap();
                std::cout << "QUEUE " << paint(game.queueString()) << '\n';
                render.printEvent(std::cout,
                                  "道具 SWAP 生效：交换待发射队首两球。");
                render.printStatus(std::cout, game);
            } catch (const std::exception &e) {
                std::cout << "ERROR " << e.what() << '\n';
                render.printErrorHint(std::cout, cmd, game, e.what());
            }
            break;
        }
        case zuma::Command::kCmdHint: {
            // HINT 是唯一"结果行替代回显行"的指令（协议 §3.2：HINT 行
            // 即结果行，避免 HINT HINT 0 的重复）——上面的回显分支已
            // 显式排除 HINT，此处只打印结果值行
            const int pos = game.hint();
            std::cout << "HINT " << pos << '\n';
            render.printHintNotice(std::cout, game, pos);
            break;
        }
        case zuma::Command::kCmdHelp: {
            render.printHelp(std::cout); // 元指令：不计步、不改状态
            break;
        }
        case zuma::Command::kCmdInvalid: {
            std::cout << "ERROR " << cmd.reason << '\n';
            render.printErrorHint(std::cout, cmd, game, cmd.reason);
            break;
        }
        default:
            break;
        }

        // 操作后终局检查（消除/发射/反转/撤销都可能改变状态）：
        // 打印终局行但不终止——UNDO 仍可回退（协议 §3.3）；
        // 只有 QUIT 是真正的结束。finalShown 防止终局后的 HELP/
        // 失败的 UNDO 把同一终局行重复打印。
        if (game.status() != zuma::Game::kPlaying && !finalShown) {
            zuma::printFinal(std::cout, game.status(), game.score(),
                             game.moves());
            finalShown = true;
        }
    }

    // EOF 或 QUIT 收尾：QUIT 已输出终局；EOF 未终局才补 QUIT_SCORE（协议 §7）
    if (!quitDone && !finalShown)
        zuma::printFinal(std::cout, zuma::Game::kQuit, game.score(),
                         game.moves());
    return 0;
}