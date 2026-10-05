#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanSwapchain {
public:
    // Pass the previous swapchain as oldSwapchain when recreating (it must still be alive)
    VulkanSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface,
        uint32_t graphicsFamily, uint32_t presentFamily, uint32_t width, uint32_t height,
        VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
    ~VulkanSwapchain();

    // Not copyable: a copy would destroy the same handles twice
    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    VkSwapchainKHR getSwapchain() const { return swapchain; }
    const std::vector<VkImage>& getImages() const { return images; }
    const std::vector<VkImageView>& getImageViews() const { return imageViews; }
    VkFormat getImageFormat() const { return imageFormat; }
    VkExtent2D getExtent() const { return extent; }

private:
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkDevice device;
    VkSurfaceKHR surface;
    uint32_t graphicsFamily;
    uint32_t presentFamily;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkFormat imageFormat;
    VkExtent2D extent;

    // Internal helpers
    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities = {}; // Initialize to default
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes);
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

    void createSwapchain(VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkSwapchainKHR oldSwapchain);
    void createImageViews();
    void destroy();
};
