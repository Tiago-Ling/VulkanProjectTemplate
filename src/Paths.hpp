#pragma once

#include <filesystem>
#include <string>

namespace Paths {
    // Directory containing the running executable (falls back to the working directory)
    std::filesystem::path executableDir();

    // Path to a compiled shader in the shaders/ folder next to the executable
    std::string shader(const std::string& name);
}
