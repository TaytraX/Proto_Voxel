#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <string>

void createTextures(std::vector<std::string> fileNames, VkCommandPool& commandPool);
VkDescriptorSetLayout getDescriptorSetLayoutTexture();
VkDescriptorSet getDescriptorSetTexture();