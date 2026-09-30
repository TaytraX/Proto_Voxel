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

    VkDescriptorPool            descriptorPool;
    VkDescriptorSet             descriptorSet;
    VkDescriptorSetLayout       descriptorSetLayout;

    VkDescriptorSet&         cameraDescriptor;
    VkDescriptorSetLayout&   cameraDescriptorLayout;

    std::vector<VkDrawIndirectCommand> commands;

    // Render pass & pipeline
    void                        createGraphicsPipeline();
    void                        createComputePipeline();
    
    void                        createBufferDescriptor();
    void					    createVertexBufferStaged();
    void                        createIndirectBuffer();
    void                        updateBuffer(glm::ivec3 chunkPos);
    void                        updateIndirectBuffer(glm::ivec3 chunkPos);
    void                        moveScene();

    void                        createDescriptorPool();
    void                        createDescriptorSetLayout();
    void                        createDescriptorSets();
    void                        setupBufferDescriptor();

public:
    SceneState(VkDescriptorSet& descriptor, VkDescriptorSetLayout& descriptorLayout);
    ~SceneState();
    void						updateChunk(glm::ivec3 chunkPos);
    void                        update();

    void                        recordCommandBuffer(VkCommandBuffer& cmdBuffer);
    void                        recordCommandFrustrum(VkCommandBuffer& cmdBuffer);
};