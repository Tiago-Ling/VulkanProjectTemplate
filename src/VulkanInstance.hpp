#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

class VulkanInstance {
public:
    VulkanInstance(const char* appName, bool enableValidation);
    ~VulkanInstance();

    VkInstance getInstance() const { return instance; }

private:
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    bool validationEnabled;

    std::vector<const char*> getRequiredExtensions();
    bool checkValidationLayerSupport();
    void createInstance(const char* appName);
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


