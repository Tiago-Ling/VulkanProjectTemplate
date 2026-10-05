#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VulkanWindow.hpp"
#include "VulkanInstance.hpp"
#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanCommand.hpp"
#include "VulkanSync.hpp"
#include "Camera.hpp"
#include "Timer.hpp"
#include "Mesh.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanImage.hpp"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;


class VulkanContext {
public:
    VulkanContext(uint32_t width, uint32_t height, const char* title);
    ~VulkanContext();
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    void run();

private:
    void init();
    void drawFrame();
    void cleanup();
    void createDepthResources();
    void recreateSwapchain();

    uint32_t width, height;
    const char* title;
    uint32_t currentFrame = 0;

    GLFWwindow* window = nullptr; // owned by vulkanWindow

    // Declared in creation order; cleanup() releases them in reverse
    std::unique_ptr<VulkanWindow> vulkanWindow;
    std::unique_ptr<VulkanInstance> instance;
    std::unique_ptr<VulkanDevice> device;
    std::unique_ptr<VulkanSwapchain> swapchain;
    std::unique_ptr<VulkanImage> depthImage;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    std::unique_ptr<VulkanPipeline> pipeline;

    // Uniform buffers and descriptor sets (one per frame in flight)
    std::vector<std::unique_ptr<VulkanBuffer>> uniformBuffers;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    std::unique_ptr<VulkanCommand> command;
    std::unique_ptr<VulkanSync> sync;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Timer> timer;
    std::unique_ptr<Mesh> mesh;
};
