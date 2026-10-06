#include "VulkanContext.hpp"
#include "Utils.hpp"
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {
    void printUsage(const char* exe) {
        std::cerr << "Usage: " << exe << " [--frames N]\n"
            << "  --frames N   Exit after rendering N frames (N > 0), e.g. for automated tests\n";
    }

    // Parses a positive decimal count; rejects signs, trailing characters and overflow
    bool parseCount(const char* text, uint64_t& out) {
        if (!std::isdigit(static_cast<unsigned char>(text[0]))) {
            return false;
        }
        char* end = nullptr;
        errno = 0;
        unsigned long long value = std::strtoull(text, &end, 10);
        if (*end != '\0' || errno != 0 || value == 0) {
            return false;
        }
        out = value;
        return true;
    }
}

int main(int argc, char** argv) {
    uint64_t frameLimit = 0; // 0 runs until the window is closed
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc && parseCount(argv[i + 1], frameLimit)) {
            ++i;
        }
        else if (std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return EXIT_SUCCESS;
        }
        else {
            std::cerr << "Invalid argument: " << argv[i] << "\n";
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    try {
        // APP_TITLE and APP_VERSION_* come from CMakeLists.txt
        VulkanContext app(800, 600, APP_TITLE,
            VK_MAKE_API_VERSION(0, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH));
        app.run(frameLimit);
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // Checked after the context is destroyed, so errors reported during cleanup are included
    if (uint32_t errors = VulkanInstance::getValidationErrorCount(); errors > 0) {
        LOG_ERROR(errors << " Vulkan validation error(s) reported; see the messages above");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
