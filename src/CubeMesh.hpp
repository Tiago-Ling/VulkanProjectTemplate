#pragma once

#include "Mesh.hpp"

class CubeMesh : public Mesh {
public:
    explicit CubeMesh(const VulkanDevice& device);
};
