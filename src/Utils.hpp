#pragma once

#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <string>
#include <vector>

// ===== Logging Macros =====
#define LOG_INFO(msg)    std::cout << "[INFO]  " << msg << std::endl
#define LOG_WARN(msg)    std::cout << "[WARN]  " << msg << std::endl
#define LOG_ERROR(msg)   std::cerr << "[ERROR] " << msg << std::endl

// ===== Vulkan Error Checker =====
// Use this to wrap Vulkan calls and check results; throws so destructors and cleanup still run
#define VK_CHECK(call) \
    do { \
        VkResult vkCheckResult = (call); \
        if (vkCheckResult != VK_SUCCESS) { \
            throw std::runtime_error(std::string(#call) + " failed with VkResult " + std::to_string(vkCheckResult)); \
        } \
    } while (0)

// ===== File Reading Utility =====
// Reads binary file into vector<char> (useful for shaders)
inline std::vector<char> readBinaryFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filename);

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

// ===== Memory Type Lookup =====
// Finds a memory type index matching the type filter and required property flags
inline uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}
