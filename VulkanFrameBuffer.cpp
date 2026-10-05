#include "VulkanFrameBuffer.hpp"
#include <stdexcept>

// Constructor: create framebuffers for each swapchain image view
VulkanFramebuffer::VulkanFramebuffer(VkDevice device,
    VkRenderPass renderPass,
    const std::vector<VkImageView>& swapchainImageViews,
    VkImageView depthImageView,
    VkExtent2D extent)
    : device(device) {
    createFramebuffers(renderPass, swapchainImageViews, depthImageView, extent);
}

// Destructor: destroy all framebuffers
VulkanFramebuffer::~VulkanFramebuffer() {
    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
}

// Create framebuffers — one per swapchain image view
void VulkanFramebuffer::createFramebuffers(VkRenderPass renderPass,
    const std::vector<VkImageView>& swapchainImageViews,
    VkImageView depthImageView,
    VkExtent2D extent) {
    framebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
        VkImageView attachments[] = {
            swapchainImageViews[i], // Color attachment (per swapchain image)
            depthImageView          // Depth attachment (shared, one frame renders at a time)
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}
