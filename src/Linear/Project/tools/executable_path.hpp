#ifndef ZUMA_TOOLS_EXECUTABLE_PATH_HPP
#define ZUMA_TOOLS_EXECUTABLE_PATH_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <stdexcept>
#include <vector>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

namespace zuma_test {

// 两个测试程序均按自身位置定位项目，支持从其他工作目录启动。
inline std::filesystem::path executableDir()
{
#ifdef __APPLE__
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0)
        throw std::runtime_error("无法定位测试程序");
    return std::filesystem::canonical(buffer.data()).parent_path();
#else
    std::vector<char> buffer(4096);
    for (;;) {
        const ssize_t n = ::readlink("/proc/self/exe", buffer.data(), buffer.size());
        if (n < 0)
            throw std::runtime_error("无法定位测试程序");
        if (static_cast<std::size_t>(n) < buffer.size())
            return std::filesystem::path(std::string(buffer.data(), n)).parent_path();
        buffer.resize(buffer.size() * 2);
    }
#endif
}

} // namespace zuma_test

#endif
