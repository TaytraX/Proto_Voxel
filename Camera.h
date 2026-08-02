#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <cstdint>

class Camera {
public:
    Camera(const glm::vec3& position, float yaw, float pitch);
    Camera() = default;

    glm::mat4 calcMatrix() const;

    glm::vec3 position;
    float yaw;   // radians
    float pitch; // radians
};

class Projection {
public:
    Projection(uint32_t width, uint32_t height, float fovyRadians, float znear, float zfar);

    void resize(uint32_t width, uint32_t height);
    glm::mat4 calcMatrix() const;

private:
    float aspect;
    float fovy; // radians
    float znear;
    float zfar;
};

// Layout std140 (doit matcher exactement la struct CameraUniform du shader Slang) :
//   float4x4 view_proj;
//   float3   view_position;
//   float    _padding;
struct CameraUniform {
    glm::mat4 viewProj;
    glm::vec3 viewPosition;
    float _padding;

    CameraUniform();
    void updateViewProj(const Camera& camera, const Projection& projection);
};

class CameraBuffer {
public:
    void create(VmaAllocator allocator);
    void destroy(VmaAllocator allocator);

    void update(const Camera& camera, const Projection& projection);

    VkBuffer getBuffer() const { return buffer; }
    VkDeviceSize getSize() const { return sizeof(CameraUniform); }

private:
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    void* mappedData = nullptr; // pointeur persistant, jamais unmap tant que le buffer vit

    CameraUniform uniformData;
};