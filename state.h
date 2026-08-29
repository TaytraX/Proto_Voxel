#pragma once
#include "shared_context.h"
#include "vma/vk_mem_alloc.h"
#include "engine_constants.hpp"
#include "mesher.h"
#include <string>
#include "Camera.h"
#include "vertex.h"
#include <vector>

extern Camera camera;

std::vector<char> readFile(const std::string& filename);

class RenderState {
	VmaAllocator&                  allocator = context.allocator;
    VkBuffer                       indirectBuffer;
    VmaAllocation                  indirectBufferAlloc;
    VkBuffer                       sceneBuffer;
    VmaAllocation   	           sceneBufferAllocation;
    VkPipelineLayout               pipelineLayout;
    VkPipelineLayout               computePipelineLayout;
    VkDescriptorSetLayout          descriptorSetLayout;
    VkDescriptorPool               descriptorPool;
    VkDescriptorSet                descriptorSet;
    VkPipeline                     graphicsPipeline;
    VkPipeline                     computePipeline;
    VkCommandPool                  commandPool;
    std::vector<VkCommandBuffer>   commandBuffers;
    std::vector<VkSemaphore>       imageAvailableSemaphores;
    std::vector<VkSemaphore>       renderFinishedSemaphores;
    std::vector<VkFence>           inFlightFences;
    uint32_t                       currentFrame = 0;
    bool                           framebufferResized = false;

    std::vector<VkDrawIndirectCommand> commands;

    // Render pass & pipeline
    void                        createGraphicsPipeline();
    void                        createComputePipeline();
    VkShaderModule              createShaderModule(const std::vector<char>& code);
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
    void                        createCommandPool();
    void                        createCommandBuffers();
    void                        recordCommandBuffer(uint32_t imageIndex);

    // Sync & draw
    void                        createSyncObjects();

public:
    RenderState();
	~RenderState();
    void						updateChunk(glm::ivec3 chunkPos, bool includePosition = false);
    void                        update();
    void                        drawFrame();
};