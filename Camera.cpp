#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

// ============================================================
// Camera
// ============================================================
Camera::Camera(const glm::vec3& position, float yaw, float pitch)
    : position(position), yaw(yaw), pitch(pitch) {
}

glm::mat4 Camera::calcMatrix() const {
    float sinPitch = std::sin(pitch);
    float cosPitch = std::cos(pitch);
    float sinYaw = std::sin(yaw);
    float cosYaw = std::cos(yaw);

    glm::vec3 direction = glm::normalize(glm::vec3(
        cosPitch * cosYaw,
        sinPitch,
        cosPitch * sinYaw
    ));

    return glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
}

// ============================================================
// Projection
// ============================================================
Projection::Projection(uint32_t width, uint32_t height, float fovyRadians, float znear, float zfar)
    : aspect(static_cast<float>(width) / static_cast<float>(height)),
    fovy(fovyRadians),
    znear(znear),
    zfar(zfar) {
}

void Projection::resize(uint32_t width, uint32_t height) {
    aspect = static_cast<float>(width) / static_cast<float>(height);
}

glm::mat4 Projection::calcMatrix() const {
    glm::mat4 proj = glm::perspective(fovy, aspect, znear, zfar);
    proj[1][1] *= -1.0f; // flip Y pour Vulkan
    return proj;
}

// ============================================================
// CameraUniform
// ============================================================
CameraUniform::CameraUniform()
    : viewProj(1.0f), viewPosition(0.0f), _padding(0.0f) {
}

void CameraUniform::updateViewProj(const Camera& camera, const Projection& projection) {
    viewPosition = camera.position;
    viewProj = projection.calcMatrix() * camera.calcMatrix();
}

#include <stdexcept>
#include <cstring>

void CameraBuffer::create(VmaAllocator allocator)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(CameraUniform);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        | VMA_ALLOCATION_CREATE_MAPPED_BIT; // reste mappé pour toute la durée de vie

    VmaAllocationInfo allocResultInfo;
    VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
        &buffer, &allocation, &allocResultInfo);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Echec de creation de l'uniform buffer camera");
    }

    mappedData = allocResultInfo.pMappedData; // valide tant que le buffer existe
}

void CameraBuffer::destroy(VmaAllocator allocator)
{
    if (buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, buffer, allocation);
        buffer = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
        mappedData = nullptr;
    }
}

void CameraBuffer::update(const Camera& camera, const Projection& projection)
{
    uniformData.updateViewProj(camera, projection);
    std::memcpy(mappedData, &uniformData, sizeof(CameraUniform));
}