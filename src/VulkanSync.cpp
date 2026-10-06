#include "VulkanSync.hpp"
#include "Utils.hpp"
#include <stdexcept>

// Constructor: create semaphores and fences
VulkanSync::VulkanSync(VkDevice device, size_t maxFramesInFlight, size_t swapchainImageCount)
    : device(device), maxFrames(maxFramesInFlight) {
    try {
        createSyncObjects();
        createImageSemaphores(swapchainImageCount);
    }
    catch (...) {
        destroy(); // the destructor does not run when the constructor throws
        throw;
    }
}

// Destructor: cleanup sync objects
VulkanSync::~VulkanSync() {
    destroy();
}

// Objects not yet created are VK_NULL_HANDLE, which the destroy calls ignore
void VulkanSync::destroy() {
    for (auto semaphore : imageAvailableSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    for (auto fence : inFlightFences) {
        vkDestroyFence(device, fence, nullptr);
    }
    destroyImageSemaphores();
}

// Create per-frame Vulkan semaphores and fences
void VulkanSync::createSyncObjects() {
    imageAvailableSemaphores.resize(maxFrames);
    inFlightFences.resize(maxFrames);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first frame doesn't wait

    for (size_t i = 0; i < maxFrames; ++i) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects!");
        }
    }
}

// Create one render-finished semaphore per swapchain image
void VulkanSync::createImageSemaphores(size_t swapchainImageCount) {
    renderFinishedSemaphores.resize(swapchainImageCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < swapchainImageCount; ++i) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects!");
        }
    }
}

void VulkanSync::destroyImageSemaphores() {
    for (auto semaphore : renderFinishedSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    renderFinishedSemaphores.clear();
}

// Caller must ensure the device is idle (done during swapchain recreation)
void VulkanSync::recreateImageSemaphores(size_t swapchainImageCount) {
    destroyImageSemaphores();
    createImageSemaphores(swapchainImageCount);
}

// Wait for fence of a given frame (throws on VK_ERROR_DEVICE_LOST and other failures)
void VulkanSync::waitForFrame(size_t frameIndex) const {
    VK_CHECK(vkWaitForFences(device, 1, &inFlightFences[frameIndex], VK_TRUE, UINT64_MAX));
}

// Reset fence after usage
void VulkanSync::resetFence(size_t frameIndex) const {
    VK_CHECK(vkResetFences(device, 1, &inFlightFences[frameIndex]));
}

// Accessors
VkSemaphore VulkanSync::getImageAvailableSemaphore(size_t frameIndex) const {
    return imageAvailableSemaphores[frameIndex];
}

VkSemaphore VulkanSync::getRenderFinishedSemaphore(size_t imageIndex) const {
    return renderFinishedSemaphores[imageIndex];
}

VkFence VulkanSync::getInFlightFence(size_t frameIndex) const {
    return inFlightFences[frameIndex];
}
