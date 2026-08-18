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
    VkBuffer                       chunkVertBuffer;
    VmaAllocation   	           chunkVertBufferAllocation;
    VkPipelineLayout               pipelineLayout;
    VkDescriptorSetLayout          descriptorSetLayout;
    VkDescriptorPool               descriptorPool;
    VkDescriptorSet                descriptorSet;
    VkPipeline                     graphicsPipeline;
    VkCommandPool                  commandPool;
    std::vector<VkCommandBuffer>   commandBuffers;
    std::vector<VkSemaphore>       imageAvailableSemaphores;
    std::vector<VkSemaphore>       renderFinishedSemaphores;
    std::vector<VkFence>           inFlightFences;
    uint32_t                       currentFrame = 0;
    bool                           framebufferResized = false;



    // Render pass & pipeline
    void                        createGraphicsPipeline();
    VkShaderModule              createShaderModule(const std::vector<char>& code);
    //void 					    createSSBO();
    uint32_t                    findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void                        createDescriptorPool();
    void                        createDescriptorSetLayout();
    void                        createDescriptorSets();
    void                        createBufferDescriptor();
    void					    createVertexBufferStaged();
    void                        createIndirectBuffer();
    void                        updateBuffer();
    void                        updateIndirectBuffer();

    void                        setupBufferDescriptor();

    // Framebuffers & commands
    void                        createCommandPool();
    void                        createCommandBuffers();
    void                        recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    // Sync & draw
    void                        createSyncObjects();

public:
    RenderState();
	~RenderState();
	void						updateChunk(std::vector<std::pair<MeshData, glm::ivec3>>* data);
    void                        update();
    void                        drawFrame();
};