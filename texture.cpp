#include "texture.h"
#include <ktx.h>
#include <array>
#include "shared_context.h"
#include <ktxvulkan.h>
#include <vma/vk_mem_alloc.h>
#include <stdexcept>

VmaAllocation allocation;


struct Texture {
	VmaAllocation allocation{ VK_NULL_HANDLE };
	VkImage image{ VK_NULL_HANDLE };
	VkImageView view{ VK_NULL_HANDLE };
	VkSampler sampler{ VK_NULL_HANDLE };
};

std::array<Texture, 2> textures;
VkBuffer imgSrcBuffer;
VmaAllocation imgSrcAllocation;
VkSampler texSampler;
VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
VkDescriptorSetLayout descriptorSetLayoutTex{ VK_NULL_HANDLE };
VkDescriptorSet descriptorSetTex{ VK_NULL_HANDLE };
std::vector<VkDescriptorImageInfo> textureDescriptors{};

void createTextures(std::vector<std::string> fileNames, VkCommandPool& commandPool) {
	for (auto i = 0; i < textures.size(); i++) {
		ktxTexture2* ktxTexture{ nullptr };
		std::string filename = "textures/" + fileNames[i] + ".ktx2";
		ktxTexture2_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
		VkImageCreateInfo texImgCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = ktxTexture2_GetVkFormat(ktxTexture),
			.extent = {.width = ktxTexture->baseWidth, .height = ktxTexture->baseHeight, .depth = 1 },
			.mipLevels = ktxTexture->numLevels,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		VmaAllocationCreateInfo texImageAllocCI{ .usage = VMA_MEMORY_USAGE_AUTO };
		vmaCreateImage(context.allocator, &texImgCI, &texImageAllocCI, &textures[i].image, &textures[i].allocation, nullptr);
		VkImageViewCreateInfo texVewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = textures[i].image, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = texImgCI.format, .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 } };
		vkCreateImageView(context.device, &texVewCI, nullptr, &textures[i].view);
		// Upload
		VkBuffer imgSrcBuffer{};
		VmaAllocation imgSrcAllocation{};
		VkBufferCreateInfo imgSrcBufferCI{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = (uint32_t)ktxTexture->dataSize, .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT };
		VmaAllocationCreateInfo imgSrcAllocCI{ .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, .usage = VMA_MEMORY_USAGE_AUTO };
		VmaAllocationInfo imgSrcAllocInfo{};
		vmaCreateBuffer(context.allocator, &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAllocation, &imgSrcAllocInfo);
		memcpy(imgSrcAllocInfo.pMappedData, ktxTexture->pData, ktxTexture->dataSize);
		VkFenceCreateInfo fenceOneTimeCI{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		VkFence fenceOneTime{};
		vkCreateFence(context.device, &fenceOneTimeCI, nullptr, &fenceOneTime);
		VkCommandBuffer cbOneTime{};
		VkCommandBufferAllocateInfo cbOneTimeAI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = commandPool, .commandBufferCount = 1 };
		vkAllocateCommandBuffers(context.device, &cbOneTimeAI, &cbOneTime);
		VkCommandBufferBeginInfo cbOneTimeBI{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI);
		VkImageMemoryBarrier2 barrierTexImage{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
			.srcAccessMask = VK_ACCESS_2_NONE,
			.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.image = textures[i].image,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
		};
		VkDependencyInfo barrierTexInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrierTexImage };
		vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
		std::vector<VkBufferImageCopy> copyRegions{};
		for (auto j = 0; j < ktxTexture->numLevels; j++) {
			ktx_size_t mipOffset{ 0 };
			KTX_error_code ret = ktxTexture2_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
			copyRegions.push_back({
				.bufferOffset = mipOffset,
				.imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)j, .layerCount = 1},
				.imageExtent{.width = ktxTexture->baseWidth >> j, .height = ktxTexture->baseHeight >> j, .depth = 1 },
				});
		}
		vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
		VkImageMemoryBarrier2 barrierTexRead{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			.image = textures[i].image,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
		};
		barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
		vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);
		vkEndCommandBuffer(cbOneTime);
		VkCommandBufferSubmitInfo cbOneTimeSubmitInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = cbOneTime };
		VkSubmitInfo2 oneTimeSI{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .commandBufferInfoCount = 1, .pCommandBufferInfos = &cbOneTimeSubmitInfo };
		vkQueueSubmit2(context.graphicsQueue, 1, &oneTimeSI, fenceOneTime);
		vkWaitForFences(context.device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX);
		vkDestroyFence(context.device, fenceOneTime, nullptr);
		vmaDestroyBuffer(context.allocator, imgSrcBuffer, imgSrcAllocation);
		// Sampler
		VkSamplerCreateInfo samplerCI{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = 16.0f,
			.maxLod = (float)ktxTexture->numLevels
		};
		vkCreateSampler(context.device, &samplerCI, nullptr, &textures[i].sampler);
		ktxTexture2_Destroy(ktxTexture);
		textureDescriptors.push_back({ .sampler = textures[i].sampler, .imageView = textures[i].view, .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL });
	}

	VkDescriptorBindingFlags descVariableFlag{ VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT };
	VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, .bindingCount = 1, .pBindingFlags = &descVariableFlag };
	VkDescriptorSetLayoutBinding descLayoutBindingTex{ .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = static_cast<uint32_t>(textures.size()), .stageFlags = VK_SHADER_STAGE_ALL };
	VkDescriptorSetLayoutCreateInfo descLayoutTexCI{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .pNext = &descBindingFlags, .bindingCount = 1, .pBindings = &descLayoutBindingTex };
	vkCreateDescriptorSetLayout(context.device, &descLayoutTexCI, nullptr, &descriptorSetLayoutTex);
	VkDescriptorPoolSize poolSize{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = static_cast<uint32_t>(textures.size()) };
	VkDescriptorPoolCreateInfo descPoolCI{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &poolSize };
	vkCreateDescriptorPool(context.device, &descPoolCI, nullptr, &descriptorPool);
	uint32_t variableDescCount{ static_cast<uint32_t>(textures.size()) };
	VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT, .descriptorSetCount = 1, .pDescriptorCounts = &variableDescCount };
	VkDescriptorSetAllocateInfo texDescSetAlloc{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .pNext = &variableDescCountAI, .descriptorPool = descriptorPool, .descriptorSetCount = 1, .pSetLayouts = &descriptorSetLayoutTex };
	vkAllocateDescriptorSets(context.device, &texDescSetAlloc, &descriptorSetTex);
	VkWriteDescriptorSet writeDescSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = descriptorSetTex, .dstBinding = 0, .descriptorCount = static_cast<uint32_t>(textureDescriptors.size()), .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = textureDescriptors.data() };
	vkUpdateDescriptorSets(context.device, 1, &writeDescSet, 0, nullptr);
}

VkDescriptorSetLayout getDescriptorSetLayoutTexture() {
    return descriptorSetLayoutTex;
}

VkDescriptorSet getDescriptorSetTexture() {
    return descriptorSetTex;
}