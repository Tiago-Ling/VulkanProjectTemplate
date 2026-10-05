cmake_minimum_required(VERSION 3.19)
project(VulkanTemplate)

# Display name used for the window title and VkApplicationInfo
set(APP_TITLE "Vulkan Template")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan SDK (also locates glslc for shader compilation)
find_package(Vulkan REQUIRED COMPONENTS glslc)

# GLFW: CMake package (vcpkg, distro packages), falling back to pkg-config
find_package(glfw3 CONFIG QUIET)
if(glfw3_FOUND)
    set(GLFW_TARGET glfw)
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED IMPORTED_TARGET glfw3)
    set(GLFW_TARGET PkgConfig::GLFW)
endif()

# GLM (header-only math library): CMake package, falling back to a header search
find_package(glm CONFIG QUIET)
if(NOT glm_FOUND)
    find_path(GLM_INCLUDE_DIR glm/glm.hpp REQUIRED)
    add_library(glm::glm INTERFACE IMPORTED)
    target_include_directories(glm::glm INTERFACE ${GLM_INCLUDE_DIR})
endif()

# Source files
set(SRC_FILES
    src/main.cpp
    src/Camera.cpp
    src/CubeMesh.cpp
    src/Input.cpp
    src/Mesh.cpp
    src/Paths.cpp
    src/ShaderLoader.cpp
    src/Timer.cpp
    src/VulkanBuffer.cpp
    src/VulkanCommand.cpp
    src/VulkanContext.cpp
    src/VulkanDevice.cpp
    src/VulkanImage.cpp
    src/VulkanInstance.cpp
    src/VulkanPipeline.cpp
    src/VulkanSwapChain.cpp
    src/VulkanSync.cpp
    src/VulkanWindow.cpp
)

# Executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE APP_TITLE="${APP_TITLE}")

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif()

# Keep the executable directly in the build directory for every generator
# (multi-config generators like Visual Studio would otherwise add Debug/ or Release/)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}>")

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link dependencies
target_link_libraries(${PROJECT_NAME}
    Vulkan::Vulkan
    ${GLFW_TARGET}
    glm::glm
    ${CMAKE_DL_LIBS}
)

# Shaders: compile GLSL to SPIR-V into shaders/ next to the executable (located at runtime via Paths.hpp)
set(SHADERS
    shaders/vertex.vert:vert.spv
    shaders/fragment.frag:frag.spv
)

set(SPV_FILES)
foreach(SHADER ${SHADERS})
    string(REPLACE ":" ";" SHADER_PAIR ${SHADER})
    list(GET SHADER_PAIR 0 SHADER_SRC)
    list(GET SHADER_PAIR 1 SHADER_SPV)
    set(SHADER_OUT ${CMAKE_BINARY_DIR}/shaders/${SHADER_SPV})
    add_custom_command(
        OUTPUT ${SHADER_OUT}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
        COMMAND Vulkan::glslc ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SRC} -o ${SHADER_OUT}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SRC}
        COMMENT "Compiling ${SHADER_SRC} -> shaders/${SHADER_SPV}"
    )
    list(APPEND SPV_FILES ${SHADER_OUT})
endforeach()

add_custom_target(shaders DEPENDS ${SPV_FILES})
add_dependencies(${PROJECT_NAME} shaders)
