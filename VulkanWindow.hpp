#pragma once

#define GLFW_INCLUDE_VULKAN // Ensure Vulkan is loaded with GLFW
#include <GLFW/glfw3.h>
#include <string>

class VulkanWindow {
public:
    // Constructor takes width, height, and window title
    VulkanWindow(uint32_t width, uint32_t height, const std::string& title);
    ~VulkanWindow();

    // Returns raw GLFW window pointer
    GLFWwindow* getGLFWWindow() const { return window; }

    // Create a Vulkan surface from the GLFW window
    VkSurfaceKHR createAndGetSurface(VkInstance instance);
	// get the Vulkan surface
	VkSurfaceKHR getSurface() const { return surface; }
	void setSurface(VkSurfaceKHR surf) { surface = surf; }

    // Framebuffer size in pixels (may differ from window size on HiDPI displays)
    void getFramebufferSize(uint32_t& outWidth, uint32_t& outHeight) const;

    // Set when the framebuffer is resized; cleared by the caller once handled
    bool wasResized() const { return framebufferResized; }
    void resetResizedFlag() { framebufferResized = false; }

    // Blocks until the window has a non-zero framebuffer (e.g. while minimized)
    void waitWhileMinimized() const;

    bool shouldClose() const;
    void pollEvents() const;
    uint32_t getWidth() const;
    uint32_t getHeight() const;

private:
    GLFWwindow* window = nullptr;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
    uint32_t width;
    uint32_t height;
    std::string title;
    bool framebufferResized = false;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};
