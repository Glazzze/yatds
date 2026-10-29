/*
 * 祖玛样例比对程序 test（阶段 4 · T4.3）
 *
 * 遍历 <项目根>/inputs/ 目录下的 .txt 样例，逐个用主程序 zuma 跑纯协议模式
 * （--plain + NO_COLOR=1，不受终端/颜色影响），与 outputs/ 下的同名
 * 标准输出逐字节比对，打印逐用例 PASS/FAIL 与汇总。
 *
 * 用法：./test          （Makefile: make check）
 * 退出码：0 全部通过；1 存在失败；2 环境/样例缺失。
 *
 * 项目根按可执行文件位置推算（支持 macOS/Linux），不依赖当前工作目录；
 * 主程序以 fork+exec 运行，stdin/stdout 重定向到文件，不经 shell。
 */
#include "executable_path.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr int kMaxDiffLines = 5; // 每个失败用例最多展示的差异行数

/* 读取整文件（二进制，保留换行原样）。 */
std::string readFile(const fs::path &path, bool &ok)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        ok = false;
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    ok = in.good() || in.eof();
    return ss.str();
}

/* 运行 driver：stdin ← input，stdout → output，环境 NO_COLOR=1。
 * 返回退出码；启动/等待失败返回 -1。 */
int runDriver(const fs::path &driver, const fs::path &input,
              const fs::path &output)
{
    const pid_t pid = ::fork();
    if (pid < 0)
        return -1;

    if (pid == 0) { // 子进程
        const int fdIn = ::open(input.c_str(), O_RDONLY);
        if (fdIn < 0)
            ::_exit(126);
        const int fdOut = ::open(output.c_str(), O_WRONLY | O_CREAT | O_TRUNC,
                                 0644);
        if (fdOut < 0)
            ::_exit(126);
        if (::dup2(fdIn, STDIN_FILENO) < 0 || ::dup2(fdOut, STDOUT_FILENO) < 0)
            ::_exit(126);
        ::close(fdIn);
        ::close(fdOut);
        ::setenv("NO_COLOR", "1", 1); // 双保险：输出不得含颜色码
        ::execl(driver.c_str(), driver.c_str(), "--plain",
                static_cast<char *>(nullptr));
        ::_exit(127);
    }

    int status = 0;
    if (::waitpid(pid, &status, 0) < 0)
        return -1;
    if (!WIFEXITED(status))
        return -1;
    return WEXITSTATUS(status);
}

fs::path makeTempFile()
{
    char name[] = "/tmp/zuma_test_XXXXXX";
    const int fd = ::mkstemp(name);
    if (fd < 0)
        return {};
    ::close(fd);
    return fs::path(name);
}

/* 按行对比，打印前 kMaxDiffLines 处差异（行号 + 期望/实际）。 */
void printDiff(const std::string &expected, const std::string &actual)
{
    std::istringstream exp(expected), act(actual);
    std::string expLine, actLine;
    int lineNo = 1;
    int shown = 0;

    while (true) {
        const bool hasExp = static_cast<bool>(std::getline(exp, expLine));
        const bool hasAct = static_cast<bool>(std::getline(act, actLine));
        if (!hasExp && !hasAct)
            break;

        if (hasExp != hasAct || expLine != actLine) {
            if (shown < kMaxDiffLines) {
                std::cout << "  第 " << lineNo << " 行:\n"
                          << "    期望: "
                          << (hasExp ? "\"" + expLine + "\"" : "<文件结束>")
                          << '\n'
                          << "    实际: "
                          << (hasAct ? "\"" + actLine + "\"" : "<文件结束>")
                          << '\n';
                ++shown;
            }
            if (hasExp != hasAct)
                break; // 一侧已结束，后续全是差异，无需逐行展开
        }
        ++lineNo;
    }

    if (shown == 0) {
        // 行内容一致但字节数不同（典型：结尾换行缺失）
        std::cout << "  行内容一致，但字节数不同：期望 " << expected.size()
                  << " 字节，实际 " << actual.size() << " 字节\n";
    } else if (shown == kMaxDiffLines) {
        std::cout << "  ...（仅显示前 " << kMaxDiffLines << " 处差异）\n";
    }
}

} // namespace

int main()
{
    const fs::path projectDir = zuma_test::executableDir();
    const fs::path inputsDir = projectDir / "inputs";
    const fs::path outputsDir = projectDir / "outputs";
    const fs::path driver = projectDir / "zuma";

    if (!fs::is_regular_file(driver)) {
        std::cerr << "找不到主程序: " << driver << "（请先运行 make）\n";
        return 2;
    }
    if (!fs::is_directory(inputsDir)) {
        std::cerr << "找不到样例目录: " << inputsDir << '\n';
        return 2;
    }
    if (!fs::is_directory(outputsDir)) {
        std::cerr << "找不到标准输出目录: " << outputsDir << '\n';
        return 2;
    }

    std::vector<fs::path> cases;
    for (const auto &entry : fs::directory_iterator(inputsDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt")
            cases.push_back(entry.path());
    }
    std::sort(cases.begin(), cases.end());

    if (cases.empty()) {
        std::cerr << "inputs/ 下没有 .txt 样例\n";
        return 2;
    }

    std::cout << "样例比对：" << cases.size() << " 个用例（"
              << inputsDir.string() << "）\n";

    int pass = 0;
    int fail = 0;
    for (const fs::path &input : cases) {
        const std::string name = input.filename().string();
        const fs::path expectedPath = outputsDir / name;

        if (!fs::is_regular_file(expectedPath)) {
            ++fail;
            std::cout << "FAIL " << name << "（缺少 outputs/" << name << "）\n";
            continue;
        }

        bool expectedOk = false;
        const std::string expected = readFile(expectedPath, expectedOk);

        const fs::path tmp = makeTempFile();
        if (!expectedOk || tmp.empty()) {
            ++fail;
            std::cout << "FAIL " << name << "（读取标准输出/创建临时文件失败）\n";
            continue;
        }

        const int rc = runDriver(driver, input, tmp);
        bool actualOk = false;
        const std::string actual = readFile(tmp, actualOk);
        fs::remove(tmp);

        if (rc != 0 || !actualOk) {
            ++fail;
            std::cout << "FAIL " << name << "（主程序运行失败，退出码 " << rc
                      << "）\n";
            continue;
        }

        if (actual == expected) {
            ++pass;
            std::cout << "PASS " << name << '\n';
        } else {
            ++fail;
            std::cout << "FAIL " << name << '\n';
            printDiff(expected, actual);
            std::cout << "------\n";
        }
    }

    std::cout << "\n汇总：PASS " << pass << " / " << cases.size();
    if (fail > 0)
        std::cout << "，FAIL " << fail;
    std::cout << '\n';
    std::cout << (fail == 0 ? "ALL SAMPLE TESTS PASSED\n"
                            : "SAMPLE TESTS FAILED\n");
    return fail == 0 ? 0 : 1;
}
