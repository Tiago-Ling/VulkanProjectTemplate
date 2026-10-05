#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <functional>
#include <string>
#include "Utils.hpp"

class VulkanDevice {
public:
    // Constructor: initializes device and queues; debug names need VK_EXT_debug_utils on the instance
    VulkanDevice(VkInstance instance, VkSurfaceKHR surface, bool enableDebugNames);

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

    // Labels a Vulkan object in validation messages and debuggers such as RenderDoc
    // (does nothing when debug names are disabled, i.e. in Release builds)
    template <typename Handle>
    void setDebugName(Handle handle, VkObjectType type, const std::string& name) const {
        if (!setObjectName) {
            return;
        }
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = type;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(handle);
        nameInfo.pObjectName = name.c_str();
        setObjectName(device, &nameInfo);
    }

private:
    // Vulkan handles
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkCommandPool uploadCommandPool = VK_NULL_HANDLE; // for immediateSubmit
    PFN_vkSetDebugUtilsObjectNameEXT setObjectName = nullptr; // null when debug names are disabled

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
