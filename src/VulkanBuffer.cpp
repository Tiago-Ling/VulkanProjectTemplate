#include "VulkanBuffer.hpp"
#include "Utils.hpp"
#include <stdexcept>
#include <cstring>

// Constructor: create buffer and allocate memory
VulkanBuffer::VulkanBuffer(VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
    : device(device), physicalDevice(physicalDevice), size(size) {
    createBuffer(size, usage, properties);
}

// Destructor: cleanup buffer and memory
VulkanBuffer::~VulkanBuffer() {
    if (mapped) {
        vkUnmapMemory(device, bufferMemory);
    }
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, buffer, nullptr);
    }
    if (bufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, bufferMemory, nullptr);
    }
}

// Create Vulkan buffer + allocate memory
void VulkanBuffer::createBuffer(VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);

    // Map host-visible memory once instead of on every write
    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        if (vkMapMemory(device, bufferMemory, 0, VK_WHOLE_SIZE, 0, &mapped) != VK_SUCCESS) {
            throw std::runtime_error("Failed to map buffer memory!");
        }
        hostCoherent = (properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
    }
}

// Copy data to buffer (used for uniforms, etc.)
void VulkanBuffer::copyData(const void* srcData, VkDeviceSize dataSize) {
    if (!mapped) {
        throw std::runtime_error("copyData() requires a host-visible buffer!");
    }
    if (dataSize > size) {
        throw std::runtime_error("copyData() size exceeds buffer size!");
    }

    std::memcpy(mapped, srcData, static_cast<size_t>(dataSize));

    if (!hostCoherent) {
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = bufferMemory;
        range.offset = 0;
        range.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges(device, 1, &range);
    }
}
