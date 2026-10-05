#pragma once

#include <vulkan/vulkan.h>

class VulkanImage {
public:
    VulkanImage(VkDevice device,
        VkPhysicalDevice physicalDevice,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties);

    ~VulkanImage();

    // Not copyable: a copy would destroy the same image, view and memory twice
    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;

    VkImage getImage() const { return image; }
    VkDeviceMemory getMemory() const { return imageMemory; }

    // Creates the image's view; it is owned and destroyed by this VulkanImage
    VkImageView createImageView(VkFormat format, VkImageAspectFlags aspectFlags);
    VkImageView getImageView() const { return imageView; }

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;

    void createImage(uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties);
    void destroy();
};
