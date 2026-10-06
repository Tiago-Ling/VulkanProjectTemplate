#pragma once

#include <vulkan/vulkan.h>
#include <atomic>
#include <vector>
#include <string>

class VulkanInstance {
public:
    VulkanInstance(const char* appName, uint32_t appVersion, bool enableValidation);
    ~VulkanInstance();

    // Not copyable: a copy would destroy the same handles twice
    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;

    VkInstance getInstance() const { return instance; }

    // Validation also enables VK_EXT_debug_utils (debug messenger and object names)
    bool isValidationEnabled() const { return validationEnabled; }

    // Validation errors reported so far by any instance, including during destruction
    static uint32_t getValidationErrorCount() { return validationErrorCount; }

private:
    static inline std::atomic<uint32_t> validationErrorCount{ 0 }; // the callback may run on driver threads

    void destroy();

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    bool validationEnabled;

    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();
    void createInstance(const char* appName, uint32_t appVersion);
    void setupDebugMessenger();
    static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
};


