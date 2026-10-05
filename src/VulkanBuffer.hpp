#pragma once

#include <vulkan/vulkan.h>

class VulkanBuffer {
public:
    VulkanBuffer(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    ~VulkanBuffer();

    // Not copyable: a copy would destroy the same buffer and memory twice
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VkBuffer getBuffer() const { return buffer; }
    VkDeviceMemory getMemory() const { return bufferMemory; }

    VkDeviceSize getSize() const { return size; }

    // Writes into a host-visible buffer through its persistent mapping (e.g. uniform data, staging)
    void copyData(const void* srcData, VkDeviceSize size);

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    void* mapped = nullptr;          // set for host-visible buffers, mapped for the buffer's lifetime
    bool hostCoherent = false;       // non-coherent memory needs an explicit flush after writes

    void createBuffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);
    void destroy();
};
