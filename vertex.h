#pragma once
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <array>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array <VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};