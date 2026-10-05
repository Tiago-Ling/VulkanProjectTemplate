#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanSync {
public:
    VulkanSync(VkDevice device, size_t maxFramesInFlight, size_t swapchainImageCount);
    ~VulkanSync();

    // Per-image semaphores must be recreated when the swapchain image count changes
    void recreateImageSemaphores(size_t swapchainImageCount);

    VkSemaphore getImageAvailableSemaphore(size_t frameIndex) const;
    VkSemaphore getRenderFinishedSemaphore(size_t imageIndex) const;
    VkFence getInFlightFence(size_t frameIndex) const;

    void waitForFrame(size_t frameIndex) const;
    void resetFence(size_t frameIndex) const;

private:
    VkDevice device;
    size_t maxFrames;

    std::vector<VkSemaphore> imageAvailableSemaphores;  // per frame in flight
    std::vector<VkSemaphore> renderFinishedSemaphores;  // per swapchain image (presentation may still hold them)
    std::vector<VkFence> inFlightFences;                // per frame in flight

    void createSyncObjects();
    void createImageSemaphores(size_t swapchainImageCount);
    void destroyImageSemaphores();
};
