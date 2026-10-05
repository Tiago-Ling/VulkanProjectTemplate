#include "VulkanDevice.hpp"
#include <stdexcept>
#include <set>
#include <vector>
#include <iostream>
#include <cstring>

namespace {
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
}

// Constructor: Pick physical device and create logical device
VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface)
    : instance(instance), surface(surface) {
    pickPhysicalDevice();
    try {
        createLogicalDevice();
    }
    catch (...) {
        destroy(); // the destructor does not run when the constructor throws
        throw;
    }
}

// Destructor: Cleanup
VulkanDevice::~VulkanDevice() {
    destroy();
}

void VulkanDevice::destroy() {
    if (uploadCommandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, uploadCommandPool, nullptr);
    }
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
    }
}

// Select a suitable physical device (GPU)
void VulkanDevice::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Pick the highest-rated suitable device (discrete GPUs first)
    int bestScore = -1;
    for (const auto& dev : devices) {
        if (!isDeviceSuitable(dev)) continue;

        int score = rateDevice(dev);
        if (score > bestScore) {
            bestScore = score;
            physicalDevice = dev;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    LOG_INFO("Using GPU: " << props.deviceName);

    queueIndices = findQueueFamilies(physicalDevice);
}


// Check if a device supports the necessary queues, extensions and surface formats
bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    return indices.isComplete()
        && checkVulkan13Support(device)
        && checkDeviceExtensionSupport(device)
        && checkSurfaceSupport(device); // queried only once the swapchain extension is confirmed
}

bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> available(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data());

    for (const char* required : deviceExtensions) {
        bool found = false;
        for (const auto& ext : available) {
            if (strcmp(required, ext.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

// Vulkan 1.3 with dynamic rendering and synchronization2 (both mandatory in 1.3, checked for safety)
bool VulkanDevice::checkVulkan13Support(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);
    if (props.apiVersion < VK_API_VERSION_1_3) {
        return false;
    }

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features13;
    vkGetPhysicalDeviceFeatures2(device, &features2);

    return features13.dynamicRendering && features13.synchronization2;
}

// The surface must offer at least one format and one present mode
bool VulkanDevice::checkSurfaceSupport(VkPhysicalDevice device) {
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    return formatCount > 0 && presentModeCount > 0;
}

// Prefer discrete GPUs, then integrated, then anything else
int VulkanDevice::rateDevice(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    switch (props.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return 3;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 2;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return 1;
    default:                                     return 0;
    }
}

// Find indices of graphics and present queue families
VulkanDevice::QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueFamilies.data());

    // Prefer a single family that supports both graphics and present
    for (uint32_t i = 0; i < queueCount; ++i) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport) {
            indices.graphicsFamily = i;
            indices.presentFamily = i;
            return indices;
        }
    }

    // Otherwise fall back to separate families
    for (uint32_t i = 0; i < queueCount; ++i) {
        if (!indices.graphicsFamily.has_value() && (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (!indices.presentFamily.has_value() && presentSupport) {
            indices.presentFamily = i;
        }
    }

    return indices;
}

// Create a logical device and retrieve queue handles
void VulkanDevice::createLogicalDevice() {
    const QueueFamilyIndices& indices = queueIndices;

    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;

    // For each required queue family, create a queue
    for (uint32_t family : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = family;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Vulkan 1.3 features used by the renderer
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    // Core features go in features2.features (e.g. samplerAnisotropy)
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features13;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features2; // replaces pEnabledFeatures
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    // Retrieve queue handles
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

    // Short-lived command buffers for setup uploads
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &uploadCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create upload command pool!");
    }
}

void VulkanDevice::immediateSubmit(const std::function<void(VkCommandBuffer)>& record) const {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = uploadCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    if (vkAllocateCommandBuffers(device, &allocInfo, &cmd) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate upload command buffer!");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    record(cmd);
    vkEndCommandBuffer(cmd);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
        vkFreeCommandBuffers(device, uploadCommandPool, 1, &cmd);
        throw std::runtime_error("Failed to create upload fence!");
    }

    VkCommandBufferSubmitInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmdInfo.commandBuffer = cmd;

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &cmdInfo;

    VkResult result = vkQueueSubmit2(graphicsQueue, 1, &submitInfo, fence);
    if (result == VK_SUCCESS) {
        vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    }

    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, uploadCommandPool, 1, &cmd);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit upload commands!");
    }
}

// Find the first supported depth format, preferring higher precision
VkFormat VulkanDevice::findDepthFormat() const {
    const VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find a supported depth format!");
}
