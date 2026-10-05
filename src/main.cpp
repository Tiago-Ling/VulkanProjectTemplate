#include "VulkanContext.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    try {
        // APP_TITLE and APP_VERSION_* come from CMakeLists.txt
        VulkanContext app(800, 600, APP_TITLE,
            VK_MAKE_API_VERSION(0, APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH));
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
