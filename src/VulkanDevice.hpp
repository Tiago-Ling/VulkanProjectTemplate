#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <functional>
#include "Utils.hpp"

class VulkanDevice {
public:
    // Constructor: initializes device and queues
    VulkanDevice(VkInstance instance, VkSurfaceKHR surface);

    // Destructor: cleans up logical device
    ~VulkanDevice();

    // Not copyable: a copy would destroy the same handles twice
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    // Accessors
    VkDevice getDevice() const { return device; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkQueue getPresentQueue() const { return presentQueue; }
    uint32_t getGraphicsQueueFamilyIndex() const {
        return queueIndices.graphicsFamily.value();
    }
    uint32_t getPresentQueueFamilyIndex() const {
        return queueIndices.presentFamily.value();
    }

    // Records commands into a one-off command buffer, submits it to the graphics queue and waits
    // for completion (for setup work such as buffer and image uploads, not per-frame rendering)
    void immediateSubmit(const std::function<void(VkCommandBuffer)>& record) const;

    // Picks a depth format usable as an optimal-tiling depth attachment
    VkFormat findDepthFormat() const;

    // Utility to find suitable memory type (used when creating buffers/images)
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
        return ::findMemoryType(physicalDevice, typeFilter, properties);
    }

private:
    // Vulkan handles
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkCommandPool uploadCommandPool = VK_NULL_HANDLE; // for immediateSubmit

    VkSurfaceKHR surface;
    VkInstance instance;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    QueueFamilyIndices queueIndices; // cached for the selected physical device

    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool checkSurfaceSupport(VkPhysicalDevice device);
    bool checkVulkan13Support(VkPhysicalDevice device);
    int rateDevice(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void createLogicalDevice();
    void destroy();

};
