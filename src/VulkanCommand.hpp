#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "Mesh.hpp"

// Images rendered into for one frame (dynamic rendering: no framebuffer object)
struct RenderTarget {
    VkImage colorImage;       // acquired swapchain image
    VkImageView colorView;
    VkImage depthImage;
    VkImageView depthView;
    VkFormat depthFormat;
    VkExtent2D extent;
};

class VulkanCommand {
public:
    // One command buffer per frame in flight, guarded by that frame's fence
    VulkanCommand(VkDevice device,
        uint32_t queueFamilyIndex,
        VkPipeline pipeline,
        size_t frameCount);

    ~VulkanCommand();

    const std::vector<VkCommandBuffer>& getCommandBuffers() const { return commandBuffers; }
    VkCommandBuffer getCommandBuffer(uint32_t index) const;

    // Records the frame's command buffer: layout transitions, rendering, transition to present
    void recordCommandBuffer(uint32_t frameIndex, const RenderTarget& target,
        Mesh* mesh, VkPipelineLayout layout, VkDescriptorSet descriptorSet);

private:
    VkDevice device;
    VkPipeline pipeline;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    void createCommandPool(uint32_t queueFamilyIndex);
    void allocateCommandBuffers(size_t count);
};
