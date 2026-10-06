#include "VulkanContext.hpp"
#include "Input.hpp"
#include "Utils.hpp"
#include "CubeMesh.hpp"
#include "Paths.hpp"
#include <stdexcept>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

struct MVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

VulkanContext::VulkanContext(uint32_t w, uint32_t h, const char* t, uint32_t version)
    : width(w), height(h), title(t), appVersion(version) {
    try {
        init();
    }
    catch (...) {
        cleanup(); // the destructor does not run when the constructor throws
        throw;
    }
}

VulkanContext::~VulkanContext() {
    cleanup();
}

void VulkanContext::init() {
    LOG_INFO("Initializing Vulkan Context...");

    vulkanWindow = std::make_unique<VulkanWindow>(width, height, title);
    window = vulkanWindow->getGLFWWindow();
    Input::init(window);

#ifdef NDEBUG
    instance = std::make_unique<VulkanInstance>(title, appVersion, false);
#else
    instance = std::make_unique<VulkanInstance>(title, appVersion, true); // skipped with a warning if the layer is missing
#endif
    vulkanWindow->createAndGetSurface(instance->getInstance());

    device = std::make_unique<VulkanDevice>(instance->getInstance(), vulkanWindow->getSurface(),
        instance->isValidationEnabled());

    uint32_t fbWidth, fbHeight;
    vulkanWindow->getFramebufferSize(fbWidth, fbHeight);
    swapchain = std::make_unique<VulkanSwapchain>(device->getPhysicalDevice(), device->getDevice(), vulkanWindow->getSurface(),
        device->getGraphicsQueueFamilyIndex(), device->getPresentQueueFamilyIndex(), fbWidth, fbHeight, ENABLE_VSYNC);

    depthFormat = device->findDepthFormat();
    createDepthResources();

    pipeline = std::make_unique<VulkanPipeline>(
        device->getDevice(),
        swapchain->getImageFormat(),
        depthFormat,
        Paths::shader("vert.spv"),
        Paths::shader("frag.spv")
    );

    // Create Uniform Buffers
    VkDeviceSize bufferSize = sizeof(MVP);
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        uniformBuffers[i] = std::make_unique<VulkanBuffer>(
            device->getDevice(),
            device->getPhysicalDevice(),
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
    }

    // Create Descriptor Pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    VK_CHECK(vkCreateDescriptorPool(device->getDevice(), &poolInfo, nullptr, &descriptorPool));

    // Create Descriptor Sets
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, pipeline->getDescriptorSetLayout());

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(vkAllocateDescriptorSets(device->getDevice(), &allocInfo, descriptorSets.data()));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i]->getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = bufferSize;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    command = std::make_unique<VulkanCommand>(
        device->getDevice(),
        device->getGraphicsQueueFamilyIndex(),
        pipeline->get(),
        MAX_FRAMES_IN_FLIGHT
    );

    sync = std::make_unique<VulkanSync>(device->getDevice(), MAX_FRAMES_IN_FLIGHT, swapchain->getImageViews().size());
    VkExtent2D extent = swapchain->getExtent();
    camera = std::make_unique<Camera>(45.0f, extent.width / (float)extent.height, 0.1f, 100.0f);
    timer = std::make_unique<Timer>();
    mesh = std::make_unique<CubeMesh>(*device);

    setDebugNames();
    setSwapchainDebugNames();

    LOG_INFO("Vulkan Context Initialized.");
}

// Names show up in validation messages and debuggers such as RenderDoc (Debug builds only)
void VulkanContext::setDebugNames() {
    device->setDebugName(device->getGraphicsQueue(), VK_OBJECT_TYPE_QUEUE, "graphics queue");
    if (device->getPresentQueue() != device->getGraphicsQueue()) {
        device->setDebugName(device->getPresentQueue(), VK_OBJECT_TYPE_QUEUE, "present queue");
    }

    device->setDebugName(pipeline->get(), VK_OBJECT_TYPE_PIPELINE, "main pipeline");
    device->setDebugName(pipeline->getLayout(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, "main pipeline layout");
    device->setDebugName(pipeline->getDescriptorSetLayout(), VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, "MVP set layout");
    device->setDebugName(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "descriptor pool");

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        const std::string frame = " [frame " + std::to_string(i) + "]";
        device->setDebugName(uniformBuffers[i]->getBuffer(), VK_OBJECT_TYPE_BUFFER, "MVP uniform buffer" + frame);
        device->setDebugName(descriptorSets[i], VK_OBJECT_TYPE_DESCRIPTOR_SET, "MVP descriptor set" + frame);
        device->setDebugName(command->getCommandBuffer(i), VK_OBJECT_TYPE_COMMAND_BUFFER, "frame commands" + frame);
        device->setDebugName(sync->getInFlightFence(i), VK_OBJECT_TYPE_FENCE, "in-flight fence" + frame);
        device->setDebugName(sync->getImageAvailableSemaphore(i), VK_OBJECT_TYPE_SEMAPHORE, "image available" + frame);
    }
}

void VulkanContext::setSwapchainDebugNames() {
    device->setDebugName(swapchain->getSwapchain(), VK_OBJECT_TYPE_SWAPCHAIN_KHR, "swapchain");
    device->setDebugName(depthImage->getImage(), VK_OBJECT_TYPE_IMAGE, "depth image");
    device->setDebugName(depthImage->getImageView(), VK_OBJECT_TYPE_IMAGE_VIEW, "depth view");

    for (size_t i = 0; i < swapchain->getImages().size(); ++i) {
        const std::string image = " [image " + std::to_string(i) + "]";
        device->setDebugName(swapchain->getImages()[i], VK_OBJECT_TYPE_IMAGE, "swapchain image" + image);
        device->setDebugName(swapchain->getImageViews()[i], VK_OBJECT_TYPE_IMAGE_VIEW, "swapchain view" + image);
        device->setDebugName(sync->getRenderFinishedSemaphore(i), VK_OBJECT_TYPE_SEMAPHORE, "render finished" + image);
    }
}

// Depth buffer matching the current swapchain extent (one is enough: frames render sequentially on one queue)
void VulkanContext::createDepthResources() {
    VkExtent2D extent = swapchain->getExtent();
    depthImage = std::make_unique<VulkanImage>(
        device->getDevice(),
        device->getPhysicalDevice(),
        extent.width,
        extent.height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    depthImage->createImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// Rebuild everything that depends on the swapchain extent (window resized, or swapchain out of date)
void VulkanContext::recreateSwapchain() {
    vulkanWindow->waitWhileMinimized();
    if (vulkanWindow->shouldClose()) {
        return;
    }

    VK_CHECK(vkDeviceWaitIdle(device->getDevice()));
    vulkanWindow->resetResizedFlag();

    uint32_t fbWidth, fbHeight;
    vulkanWindow->getFramebufferSize(fbWidth, fbHeight);

    depthImage.reset();

    // Create the new swapchain while the old one is still alive, then retire the old one
    std::unique_ptr<VulkanSwapchain> oldSwapchain = std::move(swapchain);
    swapchain = std::make_unique<VulkanSwapchain>(device->getPhysicalDevice(), device->getDevice(), vulkanWindow->getSurface(),
        device->getGraphicsQueueFamilyIndex(), device->getPresentQueueFamilyIndex(),
        fbWidth, fbHeight, ENABLE_VSYNC, oldSwapchain->getSwapchain());
    oldSwapchain.reset();

    // The pipeline is kept: the surface format does not change and viewport/scissor are dynamic
    createDepthResources();

    sync->recreateImageSemaphores(swapchain->getImageViews().size());

    VkExtent2D extent = swapchain->getExtent();
    camera->setAspectRatio(extent.width / (float)extent.height);

    setSwapchainDebugNames();
}

void VulkanContext::run(uint64_t frameLimit) {
    for (uint64_t frame = 0; !vulkanWindow->shouldClose() && (frameLimit == 0 || frame < frameLimit); ++frame) {
        Input::pollEvents(); // polls GLFW events and updates key/mouse state
        timer->update();
        drawFrame();
    }

    VK_CHECK(vkDeviceWaitIdle(device->getDevice()));
}

void VulkanContext::drawFrame() {
    size_t frameIndex = currentFrame;

    sync->waitForFrame(frameIndex);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device->getDevice(),
        swapchain->getSwapchain(),
        UINT64_MAX,
        sync->getImageAvailableSemaphore(frameIndex),
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }

    sync->resetFence(frameIndex);
    float elapsedTime = timer->getTimeSinceStart();
    float angle = glm::radians(elapsedTime * 45.0f); // 45 degrees/sec

    // Update uniform buffer (MVP)
    MVP mvp{};
    mvp.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    mvp.view = camera->getViewMatrix();
    mvp.proj = camera->getProjectionMatrix(); // already Y-flipped for Vulkan

    uniformBuffers[frameIndex]->copyData(&mvp, sizeof(MVP));

    RenderTarget target{};
    target.colorImage = swapchain->getImages()[imageIndex];
    target.colorView = swapchain->getImageViews()[imageIndex];
    target.depthImage = depthImage->getImage();
    target.depthView = depthImage->getImageView();
    target.depthFormat = depthFormat;
    target.extent = swapchain->getExtent();

    command->recordCommandBuffer(
        static_cast<uint32_t>(frameIndex),
        target,
        mesh.get(),
        pipeline->getLayout(),
        descriptorSets[frameIndex]
    );

    // Wait for the acquired image before writing color; signal when all rendering is done
    VkSemaphoreSubmitInfo waitInfo{};
    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitInfo.semaphore = sync->getImageAvailableSemaphore(frameIndex);
    waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphore signalSemaphores[] = { sync->getRenderFinishedSemaphore(imageIndex) };
    VkSemaphoreSubmitInfo signalInfo{};
    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalInfo.semaphore = signalSemaphores[0];
    signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkCommandBufferSubmitInfo cmdInfo{};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmdInfo.commandBuffer = command->getCommandBuffer(static_cast<uint32_t>(frameIndex));

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &cmdInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalInfo;

    VK_CHECK(vkQueueSubmit2(device->getGraphicsQueue(), 1, &submitInfo, sync->getInFlightFence(frameIndex)));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = { swapchain->getSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vulkanWindow->wasResized()) {
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image!");
    }
}

void VulkanContext::cleanup() {
    LOG_INFO("Cleaning up...");

    // Safe after a partial init: every step checks what was actually created
    if (device) {
        vkDeviceWaitIdle(device->getDevice()); // result ignored: cleanup runs from the destructor and must not throw
    }

    uniformBuffers.clear();

    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device->getDevice(), descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }

    mesh.reset();
    camera.reset();
    timer.reset();
    sync.reset();
    command.reset();
    pipeline.reset();
    depthImage.reset();
    swapchain.reset();
    device.reset();

    // The surface needs the instance, so it is destroyed here rather than by VulkanWindow
    if (instance && vulkanWindow && vulkanWindow->getSurface() != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance->getInstance(), vulkanWindow->getSurface(), nullptr);
        vulkanWindow->setSurface(VK_NULL_HANDLE);
    }

    vulkanWindow.reset(); // also terminates GLFW
    instance.reset();
}
