#include "VulkanWindow.hpp"
#include "Utils.hpp"
#include <stdexcept>

// Constructor: create GLFW window and initialize members
VulkanWindow::VulkanWindow(uint32_t width, uint32_t height, const std::string& title)
    : width(width), height(height), title(title) {

    // Initialize GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW!");
    }

    // Tell GLFW not to use OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create the actual window
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window!");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    LOG_INFO("GLFW window created.");
}

// Destructor: clean up
VulkanWindow::~VulkanWindow() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

// Create a Vulkan surface from GLFW window
VkSurfaceKHR VulkanWindow::createAndGetSurface(VkInstance instance) {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan window surface!");
    }
    VulkanWindow::setSurface(surface);
    return surface;
}


// Flag resizes so the swapchain gets recreated on the next frame
void VulkanWindow::framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {
    auto* self = static_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
    self->framebufferResized = true;
}

void VulkanWindow::getFramebufferSize(uint32_t& outWidth, uint32_t& outHeight) const {
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    outWidth = static_cast<uint32_t>(w);
    outHeight = static_cast<uint32_t>(h);
}

void VulkanWindow::waitWhileMinimized() const {
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    while ((w == 0 || h == 0) && !glfwWindowShouldClose(window)) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &w, &h);
    }
}

// Check if the window should close (user pressed close)
bool VulkanWindow::shouldClose() const {
    return glfwWindowShouldClose(window);
}

// Poll window/input events (must be called every frame)
void VulkanWindow::pollEvents() const {
    glfwPollEvents();
}

// Get window width
uint32_t VulkanWindow::getWidth() const {
    return width;
}

// Get window height
uint32_t VulkanWindow::getHeight() const {
    return height;
}
