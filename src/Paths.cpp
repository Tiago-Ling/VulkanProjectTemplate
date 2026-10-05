#include "Paths.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace Paths {

    std::filesystem::path executableDir() {
        std::error_code ec;
#ifdef _WIN32
        wchar_t buffer[MAX_PATH];
        DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (length > 0 && length < MAX_PATH) {
            return std::filesystem::path(buffer).parent_path();
        }
#else
        std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", ec);
        if (!ec) {
            return exe.parent_path();
        }
#endif
        return std::filesystem::current_path(ec);
    }

    std::string shader(const std::string& name) {
        return (executableDir() / "shaders" / name).string();
    }

}
