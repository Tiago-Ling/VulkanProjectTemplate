#include "VulkanInstance.hpp"
#include "Utils.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// VK_EXT_layer_settings (used for synchronization validation) needs Vulkan headers 1.3.272 or newer
#ifdef VK_EXT_layer_settings
namespace {
    // True if the given layer provides the given instance extension
    bool layerHasExtension(const char* layerName, const char* extensionName) {
        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(layerName, &count, nullptr);
        std::vector<VkExtensionProperties> available(count);
        vkEnumerateInstanceExtensionProperties(layerName, &count, available.data());

        for (const auto& ext : available) {
            if (strcmp(extensionName, ext.extensionName) == 0) {
                return true;
            }
        }
        return false;
    }
}
#endif

VulkanInstance::VulkanInstance(const char* appName, bool enableValidation)
    : validationEnabled(enableValidation) {
    if (validationEnabled && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested but not available!");
    }
    try {
        createInstance(appName);
        if (validationEnabled) {
            setupDebugMessenger();
        }
    }
    catch (...) {
        destroy(); // the destructor does not run when the constructor throws
        throw;
    }
}

VulkanInstance::~VulkanInstance() {
    destroy();
}

void VulkanInstance::destroy() {
    if (debugMessenger != VK_NULL_HANDLE) {
        auto destroyFn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroyFn) {
            destroyFn(instance, debugMessenger, nullptr);
        }
    }
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }
}

std::vector<const char*> VulkanInstance::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool VulkanInstance::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        for (const auto& layerProp : availableLayers) {
            if (strcmp(layerName, layerProp.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) return false;
    }
    return true;
}

void VulkanInstance::createInstance(const char* appName) {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    auto extensions = getRequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

#ifdef VK_EXT_layer_settings
    // Synchronization validation reports missing or incorrect barriers and semaphores
    const VkBool32 enableSyncValidation = VK_TRUE;
    VkLayerSettingEXT syncSetting{};
    syncSetting.pLayerName = validationLayers[0];
    syncSetting.pSettingName = "validate_sync";
    syncSetting.type = VK_LAYER_SETTING_TYPE_BOOL32_EXT;
    syncSetting.valueCount = 1;
    syncSetting.pValues = &enableSyncValidation;

    VkLayerSettingsCreateInfoEXT layerSettingsInfo{};
    layerSettingsInfo.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
    layerSettingsInfo.settingCount = 1;
    layerSettingsInfo.pSettings = &syncSetting;
#endif

    // Chained messenger covers messages from vkCreateInstance/vkDestroyInstance themselves
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (validationEnabled) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;

#ifdef VK_EXT_layer_settings
        if (layerHasExtension(validationLayers[0], VK_EXT_LAYER_SETTINGS_EXTENSION_NAME)) {
            extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
            debugCreateInfo.pNext = &layerSettingsInfo;
        }
        else {
            LOG_WARN("Validation layer lacks " << VK_EXT_LAYER_SETTINGS_EXTENSION_NAME
                << "; synchronization validation is off");
        }
#else
        LOG_WARN("Vulkan headers predate VK_EXT_layer_settings (1.3.272); synchronization validation is off");
#endif
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }
}

// Report validation warnings and errors only; info/verbose output is too noisy
void VulkanInstance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanInstance::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    auto createFn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (!createFn || createFn(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up debug messenger!");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* /*userData*/) {
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        LOG_ERROR("[Vulkan] " << callbackData->pMessage);
    }
    else {
        LOG_WARN("[Vulkan] " << callbackData->pMessage);
    }
    return VK_FALSE; // never abort the Vulkan call that triggered the message
}
