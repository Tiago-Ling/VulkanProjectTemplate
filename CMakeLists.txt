cmake_minimum_required(VERSION 3.19)
project(VulkanTemplate)

# Display name used for the window title and VkApplicationInfo
set(APP_TITLE "Vulkan Template")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan SDK (also locates glslc for shader compilation)
find_package(Vulkan REQUIRED COMPONENTS glslc)

# GLFW
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED IMPORTED_TARGET glfw3)

# GLM (header-only math library)
find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)

# Source files
set(SRC_FILES
    src/main.cpp
    src/Camera.cpp
    src/CubeMesh.cpp
    src/Input.cpp
    src/Mesh.cpp
    src/ShaderLoader.cpp
    src/Timer.cpp
    src/VulkanBuffer.cpp
    src/VulkanCommand.cpp
    src/VulkanContext.cpp
    src/VulkanDevice.cpp
    src/VulkanFrameBuffer.cpp
    src/VulkanImage.cpp
    src/VulkanInstance.cpp
    src/VulkanPipeline.cpp
    src/VulkanRenderPass.cpp
    src/VulkanSwapChain.cpp
    src/VulkanSync.cpp
    src/VulkanWindow.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE APP_TITLE="${APP_TITLE}")

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${GLM_INCLUDE_DIR})

# Link dependencies
target_link_libraries(${PROJECT_NAME}
    Vulkan::Vulkan
    PkgConfig::GLFW
    ${CMAKE_DL_LIBS}
)

# Shaders: compile GLSL to SPIR-V next to the executable (loaded relative to the working directory)
set(SHADERS
    shaders/vertex.vert:vert.spv
    shaders/fragment.frag:frag.spv
)

set(SPV_FILES)
foreach(SHADER ${SHADERS})
    string(REPLACE ":" ";" SHADER_PAIR ${SHADER})
    list(GET SHADER_PAIR 0 SHADER_SRC)
    list(GET SHADER_PAIR 1 SHADER_SPV)
    set(SHADER_OUT ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_SPV})
    add_custom_command(
        OUTPUT ${SHADER_OUT}
        COMMAND Vulkan::glslc ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SRC} -o ${SHADER_OUT}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SRC}
        COMMENT "Compiling ${SHADER_SRC} -> ${SHADER_SPV}"
    )
    list(APPEND SPV_FILES ${SHADER_OUT})
endforeach()

add_custom_target(shaders DEPENDS ${SPV_FILES})
add_dependencies(${PROJECT_NAME} shaders)
