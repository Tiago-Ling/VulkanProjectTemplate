#include "VulkanCommand.hpp"
#include <stdexcept>

VulkanCommand::VulkanCommand(VkDevice device,
    uint32_t queueFamilyIndex,
    VkRenderPass renderPass,
    VkPipeline pipeline,
    size_t frameCount)
    : device(device),
    renderPass(renderPass),
    pipeline(pipeline) {

    createCommandPool(queueFamilyIndex);
    allocateCommandBuffers(frameCount);
}

VulkanCommand::~VulkanCommand() {
    vkDestroyCommandPool(device, commandPool, nullptr);
}

void VulkanCommand::createCommandPool(uint32_t queueFamilyIndex) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void VulkanCommand::allocateCommandBuffers(size_t count) {
    commandBuffers.resize(count);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(count);

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

VkCommandBuffer VulkanCommand::getCommandBuffer(uint32_t index) const {
    return commandBuffers.at(index);
}

void VulkanCommand::recordCommandBuffer(
    uint32_t frameIndex,
    VkFramebuffer framebuffer,
    VkExtent2D extent,
    Mesh* mesh,
    VkPipelineLayout pipelineLayout,
    VkDescriptorSet descriptorSet)
{
    VkCommandBuffer cmd = commandBuffers[frameIndex];

    // --- Begin command buffer recording ---
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    // --- Begin render pass ---
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;

    VkClearValue clearValues[2]{};
    clearValues[0].color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // --- Bind the graphics pipeline ---
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // --- Viewport & scissor are dynamic so the pipeline survives swapchain resizes ---
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // --- Bind the descriptor set (for uniforms like MVP matrices) ---
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,                  // firstSet
        1,                  // descriptorSetCount
        &descriptorSet,
        0, nullptr          // dynamic offset count & values
    );

    // --- Bind mesh buffers (vertex + index) and issue draw command ---
    mesh->bind(cmd);
    mesh->draw(cmd);

    // --- End render pass and command buffer ---
    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}
