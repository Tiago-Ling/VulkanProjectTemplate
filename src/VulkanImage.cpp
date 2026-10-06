#include "VulkanImage.hpp"
#include "Utils.hpp"
#include <stdexcept>

// Constructor: Create the image and allocate memory
VulkanImage::VulkanImage(VkDevice device,
    VkPhysicalDevice physicalDevice,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties)
    : device(device), physicalDevice(physicalDevice) {
    try {
        createImage(width, height, format, tiling, usage, properties);
    }
    catch (...) {
        destroy(); // the destructor does not run when the constructor throws
        throw;
    }
}

// Destructor
VulkanImage::~VulkanImage() {
    destroy();
}

void VulkanImage::destroy() {
    if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    if (image != VK_NULL_HANDLE) {
        vkDestroyImage(device, image, nullptr);
    }
    if (imageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, imageMemory, nullptr);
    }
}

// Create the VkImage and allocate memory
void VulkanImage::createImage(uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;

    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory!");
    }

    VK_CHECK(vkBindImageMemory(device, image, imageMemory, 0));
}

// Create an image view for this image (color, depth, etc.)
VkImageView VulkanImage::createImageView(VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }

    return imageView;
}

