#include "VulkanContext.hpp"
#include <cstdlib>
#include <iostream>

int main() {
    try {
        VulkanContext app(800, 600, APP_TITLE);
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
