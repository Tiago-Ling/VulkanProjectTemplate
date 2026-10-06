#include "VulkanInstance.hpp"
#include "Utils.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

    // Vulkan version the layer was built for (VkLayerProperties::specVersion), or 0 if it is not installed
    uint32_t layerSpecVersion(const char* layerName) {
        uint32_t count = 0;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        std::vector<VkLayerProperties> layers(count);
        vkEnumerateInstanceLayerProperties(&count, layers.data());

        for (const auto& layer : layers) {
            if (strcmp(layerName, layer.layerName) == 0) {
                return layer.specVersion;
            }
        }
        return 0;
    }
}

VulkanInstance::VulkanInstance(const char* appName, uint32_t appVersion, bool enableValidation)
    : validationEnabled(enableValidation) {
    if (validationEnabled && !checkValidationLayerSupport()) {
        LOG_WARN("Validation layer " << validationLayers[0] << " not found (install the Vulkan SDK or "
            << "your distribution's validation layers package); continuing without validation");
        validationEnabled = false;
    }
    try {
        createInstance(appName, appVersion);
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

void VulkanInstance::createInstance(const char* appName, uint32_t appVersion) {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = appVersion;
    appInfo.pEngineName = nullptr; // set pEngineName and engineVersion when the app is built on an engine
    appInfo.engineVersion = 0;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    auto extensions = getRequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Synchronization validation reports missing or incorrect barriers and semaphores. It is off by default and
    // enabled through VK_EXT_layer_settings (Vulkan headers 1.3.272+), or else the deprecated VK_EXT_validation_features
    const VkValidationFeatureEnableEXT syncFeature = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    VkValidationFeaturesEXT validationFeatures{};
    validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeatures.enabledValidationFeatureCount = 1;
    validationFeatures.pEnabledValidationFeatures = &syncFeature;

#ifdef VK_EXT_layer_settings
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

        // Layers before 1.3.280 report false PRESENT_AFTER_WRITE hazards when a semaphore signal at ALL_COMMANDS
        // orders the transition to PRESENT_SRC (Vulkan-ValidationLayers issue #7479), so they skip sync validation
        const uint32_t layerVersion = layerSpecVersion(validationLayers[0]);
        const bool syncSupported = layerVersion >= VK_MAKE_API_VERSION(0, 1, 3, 280);

        bool syncValidation = false;
#ifdef VK_EXT_layer_settings
        if (syncSupported && layerHasExtension(validationLayers[0], VK_EXT_LAYER_SETTINGS_EXTENSION_NAME)) {
            extensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
            debugCreateInfo.pNext = &layerSettingsInfo;
            syncValidation = true;
        }
#endif
        if (syncSupported && !syncValidation
            && layerHasExtension(validationLayers[0], VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME)) {
            extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
            debugCreateInfo.pNext = &validationFeatures;
            syncValidation = true;
        }

        if (!syncSupported) {
            LOG_WARN("Validation layer " << VK_API_VERSION_MAJOR(layerVersion) << "." << VK_API_VERSION_MINOR(layerVersion)
                << "." << VK_API_VERSION_PATCH(layerVersion) << " is older than 1.3.280, which fixed false "
                << "synchronization hazards; synchronization validation is off (update the layer to enable it)");
        }
        else if (!syncValidation) {
            LOG_WARN("Validation layer supports neither " << VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME << " nor "
                << "VK_EXT_layer_settings; synchronization validation is off");
        }
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
        ++validationErrorCount;
        LOG_ERROR("[Vulkan] " << callbackData->pMessage);
    }
    else {
        LOG_WARN("[Vulkan] " << callbackData->pMessage);
    }
    return VK_FALSE; // never abort the Vulkan call that triggered the message
}
