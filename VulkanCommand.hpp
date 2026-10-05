#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Mesh.hpp"
#include "Camera.hpp"

class VulkanCommand {
public:
    // One command buffer per frame in flight, guarded by that frame's fence
    VulkanCommand(VkDevice device,
        uint32_t queueFamilyIndex,
        VkRenderPass renderPass,
        VkPipeline pipeline,
        size_t frameCount);

    ~VulkanCommand();

    const std::vector<VkCommandBuffer>& getCommandBuffers() const { return commandBuffers; }
    VkCommandBuffer getCommandBuffer(uint32_t index) const;

    // Records the frame's command buffer, targeting the acquired image's framebuffer
    void recordCommandBuffer(uint32_t frameIndex, VkFramebuffer framebuffer, VkExtent2D extent,
        Mesh* mesh, Camera* camera, VkPipelineLayout layout, VkDescriptorSet descriptorSet);



private:
    VkDevice device;
    VkRenderPass renderPass;
    VkPipeline pipeline;
    std::vector<VulkanBuffer*> uniformBuffers;
    std::vector<VkDescriptorSet> descriptorSets;
    VkDescriptorPool descriptorPool;


    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    void createCommandPool(uint32_t queueFamilyIndex);
    void allocateCommandBuffers(size_t count);
};
