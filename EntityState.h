#pragma once
#include "state.hpp"
#include <glm/glm.hpp>

extern glm::vec3* components;

class EntityState : public State
{
	VkPipeline			entityPipeline;
	VkPipelineLayout	entityPipelineLayout;

	VkBuffer		componentBuffer;
	VmaAllocation	componentBufferAlloc;

	VkDescriptorSet&             cameraDescriptorSet;
	VkDescriptorSetLayout&		 cameraDescriptorSetLayout;

	void createGraphicPipeline();
	void createInstanceBufferStaged();

public:
	EntityState(VkDescriptorSet& descriptorSett, VkDescriptorSetLayout& descriptorSetLayoutt);
	void recordCommandBuffer(VkCommandBuffer& cmdBuffer);
};