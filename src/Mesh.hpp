#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "VulkanBuffer.hpp"

class VulkanDevice;

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    static VkVertexInputBindingDescription getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

class Mesh {
public:
    // Uploads the data into device-local vertex/index buffers through a staging buffer
    Mesh(const VulkanDevice& device,
        const std::vector<Vertex>& vertices,
        const std::vector<uint32_t>& indices);

    virtual ~Mesh() = default; // deleted through Mesh* (e.g. CubeMesh)

    void bind(VkCommandBuffer commandBuffer) const;
    void draw(VkCommandBuffer commandBuffer) const;

private:
    std::unique_ptr<VulkanBuffer> vertexBuffer;
    std::unique_ptr<VulkanBuffer> indexBuffer;

    uint32_t indexCount;
};
