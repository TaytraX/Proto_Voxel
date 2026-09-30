#pragma once
#include "state.hpp"
#include "EntityState.h"
#include "SceneState.h"

class Renderer : State
{
    SceneState* sceneState;
    EntityState* entityState;

    VkDescriptorPool            descriptorPool;
    VkDescriptorSetLayout       descriptorSetLayout;
    VkDescriptorSet             descriptorSet;

    std::vector<VkSemaphore>    imageAvailableSemaphores;
    std::vector<VkSemaphore>    renderFinishedSemaphores;
    std::vector<VkFence>        inFlightFences;
    bool                        framebufferResized = false;

    void                        createDescriptorPool();
    void                        createDescriptorSetLayout();
    void                        createDescriptorSets();

    // Sync & draw
    void                        createSyncObjects();
    void recordCommandBuffer(VkCommandBuffer& cmdBuffer);
    void setupBufferDescriptor();
public:
    Renderer();
    ~Renderer();
	void drawFrame();
    void update();
};