/*
 * 祖玛 driver 集成测试（阶段 2 · T2.7；阶段 3 · T3.1–T3.3）
 *
 * 方式：内嵌（输入, 期望输出）用例表，driver 以子进程方式跑（输入经
 * 临时文件 + shell 重定向喂给 ./zuma 的 stdin），与期望逐字节比对。
 * 直接运行 ./tools/driver_test；依赖项目根的 zuma 已构建（Makefile 保证）。
 * 阶段 3 用例额外覆盖 HELP、指令名大小写、非法指令零副作用与交互渲染
 * （--ui + NO_COLOR），并断言颜色开关组合。
 *
 * 与阶段 4 的 test 程序关系：本测试内嵌样例、聚焦协议边界；阶段 4 的
 * test 读 inputs/ 与 outputs/（提交物形态），两者互补。
 */
#include "executable_path.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

int g_fail = 0;

struct Case {
    std::string name;
    std::string input;
    std::string expected;
    std::string args; // 追加给 zuma 的命令行参数（如 "--ui"）
    std::string env;  // 命令前缀环境变量（如 "NO_COLOR=1"）

    Case(std::string n, std::string in, std::string exp,
         std::string a = {}, std::string e = {})
        : name(std::move(n)), input(std::move(in)), expected(std::move(exp)),
          args(std::move(a)), env(std::move(e)) {}
};

/* 运行 driver：输入写临时文件，经 shell 重定向喂给 ./zuma 的 stdin，
 * 捕获 stdout 全文。driver 路径从可执行文件位置推算（支持 macOS/Linux，在 tools/
 * 下运行或从仓库根运行均可），不依赖当前工作目录。
 * args/env 用于验证阶段 3 的交互渲染与 NO_COLOR 开关。 */
std::string runDriver(const std::string &input, const std::string &args = "",
                      const std::string &env = "")
{
    // 输入/输出临时文件（mkstemp 保证唯一）
    char inName[] = "/tmp/zuma_drv_in.XXXXXX";
    char outName[] = "/tmp/zuma_drv_out.XXXXXX";
    int inFd = ::mkstemp(inName);
    int outFd = ::mkstemp(outName);
    if (inFd < 0 || outFd < 0) {
        std::perror("mkstemp");
        std::exit(1);
    }
    std::FILE *fin = ::fdopen(inFd, "w");
    std::fwrite(input.data(), 1, input.size(), fin);
    std::fclose(fin);
    ::close(outFd); // shell 重定向写

    // driver 路径：本程序目录的上级 + /zuma
    const std::string driver =
        (zuma_test::executableDir().parent_path() / "zuma").string();

    std::string command;
    if (!env.empty())
        command += env + " ";
    command += "\"" + driver + "\"";
    if (!args.empty())
        command += " " + args;
    command += " < " + std::string(inName) + " > " + std::string(outName);
    int rc = std::system(command.c_str());
    (void)rc;

    std::string result;
    std::FILE *fout = std::fopen(outName, "r");
    if (fout) {
        char buf[4096];
        std::size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, fout)) > 0)
            result.append(buf, n);
        std::fclose(fout);
    }
    ::unlink(inName);
    ::unlink(outName);
    return result;
}

void runCase(const Case &c)
{
    std::string actual = runDriver(c.input, c.args, c.env);
    if (actual != c.expected) {
        ++g_fail;
        std::printf("FAIL %s\n--- expected ---\n%s--- actual ---\n%s------\n",
                    c.name.c_str(), c.expected.c_str(), actual.c_str());
    } else {
        std::printf("PASS %s\n", c.name.c_str());
    }
}

/* 阶段 3 · T3.2：NO_COLOR / --color / --no-color 的组合行为。
 * 只断言"有无 ESC 与交互提示行"，避免把配色细节固化进测试。 */
void checkColorModes()
{
    const std::string input = "COLORS RB\nTRACK RB\nQUEUE R\nHINT\n";
    auto check = [](const char *name, bool ok) {
        if (ok)
            std::printf("PASS %s\n", name);
        else {
            ++g_fail;
            std::printf("FAIL %s\n", name);
        }
    };

    const std::string uiNoColor = runDriver(input, "--ui", "NO_COLOR=1");
    const std::string uiForcedOff = runDriver(input, "--ui --no-color", "");
    const std::string uiForcedOn = runDriver(input, "--ui --color", "NO_COLOR=1");
    const std::string canonical = runDriver(input, "", "");

    const std::size_t none = std::string::npos;
    check("color/no-color-env-plain",
          uiNoColor.find('\x1b') == none
              && uiNoColor.find("✦") != none); // 交互行在、颜色码不在
    check("color/--no-color", uiForcedOff.find('\x1b') == none);
    check("color/--color-overrides-NO_COLOR",
          uiForcedOn.find("\x1b[1;31m") != none // R 的红色
              && uiForcedOn.find("✦") != none);
    check("color/canonical-purity", canonical.find('\x1b') == none
                                        && canonical.find("✦") == none);
}

} // namespace

int main()
{
    std::vector<Case> cases = {
        /* 01 README 示例轨道：装载连锁归空（装载消除不计分）→ WIN */
        {"load-win",
         "COLORS RB\nTRACK RRBBBRR\nQUEUE B\n",
         "COLORS RB\nTRACK RRBBBRR\nQUEUE B\nREADY\nWIN SCORE=0 MOVES=0\n"},

        /* 02 单发射三连消除 + 计分 + 队列输出 */
        {"shoot-elim",
         "COLORS RB\nTRACK RR\nQUEUE RB\nSHOOT 2\n",
         "COLORS RB\nTRACK RR\nQUEUE RB\nREADY\n"
         "SHOOT 2\n"
         "ELIM round=1 left=0 count=3 balls=RRR score=+30\n"
         "TRACK \n"
         "QUEUE B\n"
         "WIN SCORE=30 MOVES=1\n"},

        /* 03 UNDO 完整回滚（消除回填 + 球回队首 + 重放） */
        {"undo-replay",
         "COLORS RB\nTRACK RRBBRR\nQUEUE BB\nSHOOT 4\nUNDO\nSHOOT 4\n",
         "COLORS RB\nTRACK RRBBRR\nQUEUE BB\nREADY\n"
         "SHOOT 4\n"
         "ELIM round=1 left=2 count=3 balls=BBB score=+30\n"
         "TRACK RRRR\n"
         "ELIM round=2 left=0 count=4 balls=RRRR score=+80\n"
         "TRACK \n"
         "QUEUE B\n"
         "WIN SCORE=110 MOVES=1\n"
         "UNDO\n"
         "TRACK RRBBRR\n"
         "QUEUE BB\n"
         "SHOOT 4\n"
         "ELIM round=1 left=2 count=3 balls=BBB score=+30\n"
         "TRACK RRRR\n"
         "ELIM round=2 left=0 count=4 balls=RRRR score=+80\n"
         "TRACK \n"
         "QUEUE B\n"
         "WIN SCORE=110 MOVES=1\n"},

        /* 04 变色球未定色 → ERROR 不改状态（EOF 后按退出计） */
        {"chameleon",
         "COLORS RB\nTRACK RR\nQUEUE ?R\nSHOOT 2\n",
         "COLORS RB\nTRACK RR\nQUEUE ?R\nREADY\n"
         "SHOOT 2\n"
         "ERROR 变色球必须指定 COLORS 内的颜色\n"
         "QUIT_SCORE SCORE=0 MOVES=0\n"},

        /* 04b 变色球定色成功 */
        {"chameleon-color",
         "COLORS RB\nTRACK RR\nQUEUE ?\nSHOOT 2 R\n",
         "COLORS RB\nTRACK RR\nQUEUE ?\nREADY\n"
         "SHOOT 2 R\n"
         "ELIM round=1 left=0 count=3 balls=RRR score=+30\n"
         "TRACK \n"
         "QUEUE \n"
         "WIN SCORE=30 MOVES=1\n"},

        /* 05 炸弹球：R*GB → 消 R*G 剩 B；队列空轨道非空 → LOSE */
        {"bomb",
         "COLORS RGB\nTRACK RGB\nQUEUE *\nSHOOT 1\n",
         "COLORS RGB\nTRACK RGB\nQUEUE *\nREADY\n"
         "SHOOT 1\n"
         "ELIM round=1 left=0 count=3 balls=R*G score=+30\n"
         "TRACK B\n"
         "QUEUE \n"
         "LOSE SCORE=30 MOVES=1\n"},

        /* 06 SWAP；队列耗尽后 SWAP 不足两球 → ERROR */
        {"swap",
         "COLORS RB\nTRACK R\nQUEUE RB\nSWAP\nSHOOT 0\nSWAP\n",
         "COLORS RB\nTRACK R\nQUEUE RB\nREADY\n"
         "SWAP\nQUEUE BR\n"
         "SHOOT 0\nTRACK BR\nQUEUE R\n"
         "SWAP\nERROR 队列不足两球，无法交换\n"
         "QUIT_SCORE SCORE=0 MOVES=2\n"},

        /* 07 REVERSE 拼出四连消除（RRBBRR 反转 [2,5] → RRRRBB 消）+ 撤销 */
        {"reverse",
         "COLORS RB\nTRACK RRBBRR\nQUEUE R\nREVERSE 2 5\nUNDO\n",
         "COLORS RB\nTRACK RRBBRR\nQUEUE R\nREADY\n"
         "REVERSE 2 5\n"
         "ELIM round=1 left=0 count=4 balls=RRRR score=+40\n"
         "TRACK BB\n"
         "UNDO\n"
         "TRACK RRBBRR\n"
         "QUEUE R\n"
         "QUIT_SCORE SCORE=0 MOVES=0\n"},

        /* 08 HINT：等消除数取最左（结果行即 HINT 行） */
        {"hint",
         "COLORS RB\nTRACK RRB\nQUEUE R\nHINT\n",
         "COLORS RB\nTRACK RRB\nQUEUE R\nREADY\n"
         "HINT 0\n"
         "QUIT_SCORE SCORE=0 MOVES=0\n"},

        /* 09 LIMIT 判负（含中途消除仍判负：步数到限） */
        {"limit",
         "COLORS RB\nTRACK RB\nQUEUE RR\nLIMIT 2\nSHOOT 0\nSHOOT 0\n",
         "COLORS RB\nTRACK RB\nQUEUE RR\nLIMIT 2\nREADY\n"
         "SHOOT 0\nTRACK RRB\nQUEUE R\n"
         "SHOOT 0\n"
         "ELIM round=1 left=0 count=3 balls=RRR score=+30\n"
         "TRACK B\n"
         "QUEUE \n"
         "LOSE SCORE=30 MOVES=2\n"},

        /* 10 非法指令全表（语法错 + 语义错，均不改状态、不计步） */
        {"invalid",
         "COLORS RB\nTRACK R\nQUEUE R\nSHOOT\nSHOOT X\nSHOOT 1 2\n"
         "REVERSE 0\nFOO\nUNDO\n",
         "COLORS RB\nTRACK R\nQUEUE R\nREADY\n"
         "SHOOT\nERROR SHOOT 缺少位置参数\n"
         "SHOOT X\nERROR SHOOT 位置不是非负整数\n"
         "SHOOT 1 2\nERROR SHOOT 参数格式错误（<pos> 或 <pos> <大写字母>）\n"
         "REVERSE 0\nERROR REVERSE 需要 <i> <j> 两个参数\n"
         "FOO\nERROR 未知指令或参数数量错误\n"
         "UNDO\nERROR 撤销栈为空\n"
         "QUIT_SCORE SCORE=0 MOVES=0\n"},

        /* 11 越界发射：ERROR 且不改状态 */
        {"out-of-range",
         "COLORS RB\nTRACK R\nQUEUE R\nSHOOT 99\n",
         "COLORS RB\nTRACK R\nQUEUE R\nREADY\n"
         "SHOOT 99\nERROR 发射位置越界\n"
         "QUIT_SCORE SCORE=0 MOVES=0\n"},

        /* 12 EOF 无 QUIT：按退出计 */
        {"eof-quit",
         "COLORS RB\nTRACK RB\nQUEUE R\n",
         "COLORS RB\nTRACK RB\nQUEUE R\nREADY\n"
         "QUIT_SCORE SCORE=0 MOVES=0\n"},

        /* 13 空行与注释：静默忽略 */
        {"comments",
         "COLORS RB\nTRACK R\nQUEUE R\n\n# 注释\nSHOOT 1\n",
         "COLORS RB\nTRACK R\nQUEUE R\nREADY\n"
         "SHOOT 1\nTRACK RR\nQUEUE \n"
         "LOSE SCORE=0 MOVES=1\n"},

        /* 14 终局后指令：回显但不执行（防御路径）——WIN 后多余 SHOOT */
        {"after-final",
         "COLORS RB\nTRACK RR\nQUEUE R\nSHOOT 2\nSHOOT 0\n",
         "COLORS RB\nTRACK RR\nQUEUE R\nREADY\n"
         "SHOOT 2\n"
         "ELIM round=1 left=0 count=3 balls=RRR score=+30\n"
         "TRACK \n"
         "QUEUE \n"
         "WIN SCORE=30 MOVES=1\n"
         "SHOOT 0\n"
         "ERROR 终局后不接受指令\n"},

        /* 15 阶段 3：HELP 指令（协议 v1.1，元指令不计步） */
        {"help",
         "COLORS RB\nTRACK R\nQUEUE R\nHELP\n",
         "COLORS RB\nTRACK R\nQUEUE R\nREADY\n"
         "HELP\n"
         "可用指令：\n"
         "  SHOOT <pos> [color]     发射队首球到下标 pos（0 起，p=n 为尾插）\n"
         "  UNDO                    撤销上一步（含连锁回滚，可撤销终局步）\n"
         "  SWAP                    交换待发射队列的头两球\n"
         "  HINT                    提示队首球的最优插入位置（-1 = 无可消除位）\n"
         "  REVERSE <i> <j>         反转轨道闭区间 [i, j]（0 <= i <= j < n）\n"
         "  HELP                    显示本帮助（不计步数）\n"
         "  QUIT                    退出并输出终局\n"
         "特殊球：* 炸弹（消除邻域至多 3 球）  ? 变色（SHOOT 时须指定颜色）\n"
         "位置：0-based，从轨道左端起；SHOOT <pos> 表示插入后新球位于下标 pos。\n"
         "QUIT_SCORE SCORE=0 MOVES=0\n"},

        /* 16 阶段 3：指令名不区分大小写（回显保留原始大小写） */
        {"lowercase-cmd",
         "COLORS RB\nTRACK R\nQUEUE R\nshoot 1\n",
         "COLORS RB\nTRACK R\nQUEUE R\nREADY\n"
         "shoot 1\nTRACK RR\nQUEUE \n"
         "LOSE SCORE=0 MOVES=1\n"},

        /* 17 阶段 3：变色球颜色非法时不得吞掉队首球（非法指令零副作用） */
        {"chameleon-invalid-color",
         "COLORS RB\nTRACK R\nQUEUE ?R\nSHOOT 0 X\nSHOOT 0 R\n",
         "COLORS RB\nTRACK R\nQUEUE ?R\nREADY\n"
         "SHOOT 0 X\nERROR 变色球必须指定 COLORS 内的颜色\n"
         "SHOOT 0 R\nTRACK RR\nQUEUE R\n"
         "QUIT_SCORE SCORE=0 MOVES=1\n"},

        /* 18 阶段 3：终局后 HELP 仍可用，且终局行不得重复打印 */
        {"help-after-final",
         "COLORS RB\nTRACK RR\nQUEUE R\nSHOOT 2\nHELP\n",
         "COLORS RB\nTRACK RR\nQUEUE R\nREADY\n"
         "SHOOT 2\n"
         "ELIM round=1 left=0 count=3 balls=RRR score=+30\n"
         "TRACK \n"
         "QUEUE \n"
         "WIN SCORE=30 MOVES=1\n"
         "HELP\n"
         "可用指令：\n"
         "  SHOOT <pos> [color]     发射队首球到下标 pos（0 起，p=n 为尾插）\n"
         "  UNDO                    撤销上一步（含连锁回滚，可撤销终局步）\n"
         "  SWAP                    交换待发射队列的头两球\n"
         "  HINT                    提示队首球的最优插入位置（-1 = 无可消除位）\n"
         "  REVERSE <i> <j>         反转轨道闭区间 [i, j]（0 <= i <= j < n）\n"
         "  HELP                    显示本帮助（不计步数）\n"
         "  QUIT                    退出并输出终局\n"
         "特殊球：* 炸弹（消除邻域至多 3 球）  ? 变色（SHOOT 时须指定颜色）\n"
         "位置：0-based，从轨道左端起；SHOOT <pos> 表示插入后新球位于下标 pos。\n"},

        /* 19 阶段 3：交互渲染（T3.1/T3.3）——NO_COLOR 下纯文本，
         * 状态摘要/事件提示/错误建议逐行可比对 */
        {"ui-render",
         "COLORS RB\nTRACK RRB\nQUEUE RB\nSHOOT 2\nSHOOT 9\nHINT\nUNDO\n",
         "═══ 祖玛 · CLI 模式 ═══\n"
         "  帮助: HELP · 退出: QUIT · 关闭颜色: NO_COLOR=1\n"
         "COLORS RB\nTRACK RRB\nQUEUE RB\nREADY\n"
         "  · 分数 0 · 步数 0 · 轨道 3 球 · 队列 2 球\n"
         "SHOOT 2\n"
         "ELIM round=1 left=0 count=3 balls=RRR score=+30\n"
         "TRACK B\n"
         "  ✦ 消除 3 球，+30 分\n"
         "QUEUE B\n"
         "  · 分数 30 · 步数 1 · 轨道 1 球 · 队列 1 球\n"
         "SHOOT 9\n"
         "ERROR 发射位置越界\n"
         "  ▸ 提示：位置越界——轨道共 1 球，可插入位置为 0..1"
         "（0 为最左端，1 为尾插）。\n"
         "HINT -1\n"
         "  ✦ 提示：当前没有可立即消除的位置。\n"
         "UNDO\n"
         "TRACK RRB\n"
         "QUEUE RB\n"
         "  ✦ 已撤销上一步，分数与步数同步回滚。\n"
         "  · 分数 0 · 步数 0 · 轨道 3 球 · 队列 2 球\n"
         "QUIT_SCORE SCORE=0 MOVES=0\n",
         "--ui",
         "NO_COLOR=1"},

        /* 20 阶段 3：交互渲染的连锁提示（消除 → 连锁 ×2） */
        {"ui-chain",
         "COLORS RB\nTRACK RRBBRR\nQUEUE BB\nSHOOT 4\n",
         "═══ 祖玛 · CLI 模式 ═══\n"
         "  帮助: HELP · 退出: QUIT · 关闭颜色: NO_COLOR=1\n"
         "COLORS RB\nTRACK RRBBRR\nQUEUE BB\nREADY\n"
         "  · 分数 0 · 步数 0 · 轨道 6 球 · 队列 2 球\n"
         "SHOOT 4\n"
         "ELIM round=1 left=2 count=3 balls=BBB score=+30\n"
         "TRACK RRRR\n"
         "  ✦ 消除 3 球，+30 分\n"
         "ELIM round=2 left=0 count=4 balls=RRRR score=+80\n"
         "TRACK \n"
         "  ✦ 连锁 ×2：再消除 4 球，+80 分\n"
         "QUEUE B\n"
         "  · 分数 110 · 步数 1 · 轨道 0 球 · 队列 1 球\n"
         "WIN SCORE=110 MOVES=1\n",
         "--ui",
         "NO_COLOR=1"},
    };

    // 复用现有固定 HELP 正文期望，避免测试之间复制整段帮助文本。
    std::string helpText;
    for (const Case &c : cases) {
        if (c.name == "help") {
            const auto start = c.expected.find("HELP\n");
            helpText = c.expected.substr(start, c.expected.find("QUIT_SCORE", start) - start);
        }
    }
    cases.emplace_back("quit-stops-with-history",
        "COLORS RB\nTRACK RB\nQUEUE RRB\nSHOOT 0\nQUIT\nUNDO\nHELP\nQUIT\n",
        "COLORS RB\nTRACK RB\nQUEUE RRB\nREADY\nSHOOT 0\nTRACK RRB\nQUEUE RB\n"
        "QUIT_SCORE SCORE=0 MOVES=1\n");
    cases.emplace_back("quit-stops-immediately",
        "COLORS RB\nTRACK RB\nQUEUE R\nQUIT\nUNDO\nQUIT\n",
        "COLORS RB\nTRACK RB\nQUEUE R\nREADY\nQUIT_SCORE SCORE=0 MOVES=0\n");
    cases.emplace_back("win-hint-echo",
        "COLORS RB\nTRACK RR\nQUEUE R\nSHOOT 0\nhint\n",
        "COLORS RB\nTRACK RR\nQUEUE R\nREADY\nSHOOT 0\n"
        "ELIM round=1 left=0 count=3 balls=RRR score=+30\nTRACK \nQUEUE \n"
        "WIN SCORE=30 MOVES=1\nhint\nERROR 终局后不接受指令\n");
    cases.emplace_back("loaded-win-help",
        "COLORS RB\nTRACK\nQUEUE R\nHELP\nHINT\nUNDO\n",
        "COLORS RB\nTRACK \nQUEUE R\nREADY\nWIN SCORE=0 MOVES=0\n" + helpText +
        "HINT\nERROR 终局后不接受指令\nUNDO\nERROR 撤销栈为空\n");
    cases.emplace_back("loaded-lose-help",
        "COLORS RB\nTRACK RB\nQUEUE\nHELP\nHINT\n",
        "COLORS RB\nTRACK RB\nQUEUE \nREADY\nLOSE SCORE=0 MOVES=0\n" + helpText +
        "HINT\nERROR 终局后不接受指令\n");
    cases.emplace_back("bomb-hint-endpoint",
        "COLORS RGB\nTRACK RGB\nQUEUE *R\nHINT\n",
        "COLORS RGB\nTRACK RGB\nQUEUE *R\nREADY\nHINT 0\nQUIT_SCORE SCORE=0 MOVES=0\n");

    for (const Case &c : cases)
        runCase(c);

    checkColorModes();

    if (g_fail) {
        std::printf("%d 个用例失败\n", g_fail);
        return 1;
    }
    std::printf("ALL DRIVER TESTS PASSED (%zu 用例)\n", cases.size());
    return 0;
}
