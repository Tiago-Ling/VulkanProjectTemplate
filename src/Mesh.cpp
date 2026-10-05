#include "Mesh.hpp"
#include "VulkanDevice.hpp"
#include <cstring>

namespace {
    // Creates a device-local buffer and fills it via a host-visible staging buffer
    std::unique_ptr<VulkanBuffer> createDeviceLocalBuffer(const VulkanDevice& device,
        const void* data, VkDeviceSize size, VkBufferUsageFlags usage,
        VkPipelineStageFlags2 dstStage, VkAccessFlags2 dstAccess) {
        VulkanBuffer staging(
            device.getDevice(), device.getPhysicalDevice(),
            size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        staging.copyData(data, size);

        auto buffer = std::make_unique<VulkanBuffer>(
            device.getDevice(), device.getPhysicalDevice(),
            size,
            usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        device.immediateSubmit([&](VkCommandBuffer cmd) {
            VkBufferCopy region{};
            region.size = size;
            vkCmdCopyBuffer(cmd, staging.getBuffer(), buffer->getBuffer(), 1, &region);

            // Make the copy visible to the stage that reads the buffer in later submissions
            VkBufferMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            barrier.dstStageMask = dstStage;
            barrier.dstAccessMask = dstAccess;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = buffer->getBuffer();
            barrier.offset = 0;
            barrier.size = VK_WHOLE_SIZE;

            VkDependencyInfo dependency{};
            dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependency.bufferMemoryBarrierCount = 1;
            dependency.pBufferMemoryBarriers = &barrier;
            vkCmdPipelineBarrier2(cmd, &dependency);
        });

        return buffer;
    }
}

// Vertex input binding for Vulkan pipeline
VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding;
}

// Describe layout of Vertex attributes (position and color)
std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attrs(2);

    attrs[0].binding = 0;
    attrs[0].location = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(Vertex, position);

    attrs[1].binding = 0;
    attrs[1].location = 1;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = offsetof(Vertex, color);

    return attrs;
}


// Constructor: Upload vertex and index data to GPU buffers
Mesh::Mesh(const VulkanDevice& device,
    const std::vector<Vertex>& vertices,
    const std::vector<uint32_t>& indices,
    const std::string& debugName)
    : indexCount(static_cast<uint32_t>(indices.size())) {

    vertexBuffer = createDeviceLocalBuffer(device,
        vertices.data(), sizeof(vertices[0]) * vertices.size(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT);

    indexBuffer = createDeviceLocalBuffer(device,
        indices.data(), sizeof(indices[0]) * indices.size(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, VK_ACCESS_2_INDEX_READ_BIT);

    device.setDebugName(vertexBuffer->getBuffer(), VK_OBJECT_TYPE_BUFFER, debugName + " vertices");
    device.setDebugName(indexBuffer->getBuffer(), VK_OBJECT_TYPE_BUFFER, debugName + " indices");
}

// Bind vertex and index buffers for drawing
void Mesh::bind(VkCommandBuffer commandBuffer) const {
    VkBuffer vertexBuf = vertexBuffer->getBuffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuf, &offset);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

// Issue draw command for the mesh
void Mesh::draw(VkCommandBuffer commandBuffer) const {
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
}
