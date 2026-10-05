#include "ShaderLoader.hpp"
#include "Utils.hpp"
#include <fstream>
#include <stdexcept>

// Read a binary .spv file and return its contents
std::vector<char> ShaderLoader::readSPIRV(const std::string& filename) {
    return readBinaryFile(filename);
}

// Create a Vulkan shader module from binary SPIR-V code
VkShaderModule ShaderLoader::createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // must be aligned

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}
