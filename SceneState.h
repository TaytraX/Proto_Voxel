#pragma once
#include "state.hpp"
#include "engine_constants.hpp"
#include "mesher.h"
#include <string>
#include "Camera.h"
#include <vector>

extern Camera camera;

class SceneState : State {
    VkPipeline                  graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout            pipelineLayout = VK_NULL_HANDLE;
    VkPipeline                  sceneMovePipeline = VK_NULL_HANDLE;
    VkPipelineLayout            sceneMovePipelineLayout = VK_NULL_HANDLE;
    VkPipeline                  frustrumPipeline = VK_NULL_HANDLE;
    VkPipelineLayout            frustrumPipelineLayout = VK_NULL_HANDLE;

    VkBuffer                    indirectBuffer = VK_NULL_HANDLE;
    VmaAllocation               indirectBufferAlloc = VK_NULL_HANDLE;
    std::vector<VkBuffer>       sceneBuffer;
    std::vector<VmaAllocation>  sceneBufferAllocation;
    VkDescriptorSetLayout       descriptorSetLayout;
    VkDescriptorPool            descriptorPool;
    VkDescriptorSet             descriptorSet;

    std::vector<VkSemaphore>    imageAvailableSemaphores;
    std::vector<VkSemaphore>    renderFinishedSemaphores;
    std::vector<VkFence>        inFlightFences;
    bool                        framebufferResized = false;

    std::vector<VkDrawIndirectCommand> commands;

    // Render pass & pipeline
    void                        createGraphicsPipeline();
    void                        createComputePipeline();
    
    uint32_t                    findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void                        createDescriptorPool();
    void                        createDescriptorSetLayout();
    void                        createDescriptorSets();
    void                        createBufferDescriptor();
    void					    createVertexBufferStaged();
    void                        createIndirectBuffer();
    void                        updateBuffer(glm::ivec3 chunkPos);
    void                        updateIndirectBuffer(glm::ivec3 chunkPos);
    void                        moveScene();

    void                        setupBufferDescriptor();

    // Framebuffers & commands
    void                        recordCommandBuffer();

    // Sync & draw
    void                        createSyncObjects();

public:
    SceneState();
    ~SceneState();
    void						updateChunk(glm::ivec3 chunkPos);
    void                        update();
    void                        drawFrame();
};