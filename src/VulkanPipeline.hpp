#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class VulkanPipeline {
public:
    VulkanPipeline(VkDevice device,
        VkFormat colorFormat,
        VkFormat depthFormat,
        const std::string& vertShaderPath,
        const std::string& fragShaderPath);

    ~VulkanPipeline();

    // Not copyable: a copy would destroy the same handles twice
    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    VkPipeline get() const { return pipeline; }
    VkPipelineLayout getLayout() const { return pipelineLayout; }
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

private:
    VkDevice device;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    void createGraphicsPipeline(VkFormat colorFormat,
        VkFormat depthFormat,
        const std::string& vertShaderPath,
        const std::string& fragShaderPath);
    void destroy();
};
