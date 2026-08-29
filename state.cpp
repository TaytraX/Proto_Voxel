#include "state.h"
#include "texture.h"
#include <fstream>
#include <iostream>
#include "scene.h"

Camera 					       camera(glm::vec3(32.0f, 1.0f, 32.0f), glm::radians(90.0f), glm::radians(0.0f));
Projection 			           projection(800, 600, glm::radians(45.0f), 0.1f, (RENDER_DISTANCE * CHUNK_AXIS1_SIZE) * 1.5f * sqrt(2.0f));
CameraUniform				   cameraUniform;
CameraBuffer				   cameraUniformBuffer;

// ── pipeline ────────────────────────────────────────────────────

void RenderState::createGraphicsPipeline() {
    auto shaderCode = readFile("shaders/build/shader.spv");

    VkShaderModule shaderModule = createShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = shaderModule;
    vertShaderStageInfo.pName = "vs_main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = shaderModule;
    fragShaderStageInfo.pName = "ps_main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(uint64_t);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    VkVertexInputAttributeDescription attributeDescription{
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32_UINT
    };

    VkPipelineVertexInputStateCreateInfo vertexInputState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &attributeDescription
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineDepthStencilStateCreateInfo depthStencilState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };

    VkPipelineRenderingCreateInfo renderingCI{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &context.swapChainImageFormat,
    .depthAttachmentFormat = context.depthFormat
    };

    auto descTexLayout = getDescriptorSetLayoutTexture();

    auto layouts = {
        descriptorSetLayout,
        descTexLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = (uint32_t)layouts.size(),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 0
    };

    if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputState;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.pNext = &renderingCI;

    if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(context.device, shaderModule, nullptr);
}

void RenderState::createComputePipeline() {
    auto shaderCode = readFile("shaders/build/scene_update.spv");

    VkShaderModule shaderModule = createShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = shaderModule;
    computeShaderStageInfo.pName = "move_scene";

    auto layouts = {
        descriptorSetLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = (uint32_t)layouts.size(),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 0
    };

    if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline layout!");
    }

    VkComputePipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = computeShaderStageInfo,
        .layout = computePipelineLayout,
    };

    if (vkCreateComputePipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }

    vkDestroyShaderModule(context.device, shaderModule, nullptr);
}

VkShaderModule RenderState::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(context.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

// ── commands ───────────────────────────────────────────────────

void RenderState::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = context.findQueueFamilies(context.physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(context.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void RenderState::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(context.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void RenderState::recordCommandBuffer(uint32_t imageIndex) {
    VkCommandBuffer& commandBuffer = commandBuffers[currentFrame];
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

    VkRenderingAttachmentInfo colorAttachmentInfo{
    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = context.swapChainImageViews[imageIndex],
    .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .clearValue{clearColor}
    };


    VkRenderingAttachmentInfo depthAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = context.depthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = {.depthStencil = {1.0f,  0}}
    };

    VkRenderingInfo renderingInfo{
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea{
            .offset = {0, 0},
            .extent = context.swapChainExtent,
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentInfo,
    .pDepthAttachment = &depthAttachmentInfo
    };
    VkDescriptorSet textSet = getDescriptorSetTexture();
    VkDescriptorSet set[2] = { descriptorSet, textSet };

    std::array<VkImageMemoryBarrier2, 2> outputBarriers{
    VkImageMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .image = context.swapChainImages[imageIndex],
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
    },
    VkImageMemoryBarrier2{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .image = context.depthImage,
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 }
    }
    };
    VkDependencyInfo barrierPresentDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 2,
        .pImageMemoryBarriers = outputBarriers.data()
    };
    vkCmdPipelineBarrier2(commandBuffer, &barrierPresentDependencyInfo);

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)context.swapChainExtent.width;
    viewport.height = (float)context.swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = context.swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, set, 0, nullptr);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sceneBuffer, offsets);
    vkCmdDrawIndirect(commandBuffer, indirectBuffer, 0, 6 * (uint32_t)scene::chunkMap.size(), sizeof(VkDrawIndirectCommand));

    vkCmdEndRendering(commandBuffer);

    VkImageMemoryBarrier2 toPresent{
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
    .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
    .dstAccessMask = VK_ACCESS_2_NONE,
    .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    .image = context.swapChainImages[imageIndex],
    .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    };
    VkDependencyInfo depInfoOut{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &toPresent };
    vkCmdPipelineBarrier2(commandBuffer, &depInfoOut);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

// ── Sync objects & draw ───────────────────────────────────────────────────────

void RenderState::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(context.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void RenderState::drawFrame() {
    vkWaitForFences(context.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(context.device, context.swapChain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        context.recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(context.device, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);

    recordCommandBuffer(imageIndex);

    VkSemaphore          waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore          signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = { context.swapChain };

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
        .pSwapchains = swapChains,
		.pImageIndices = &imageIndex
    };

    result = vkQueuePresentKHR(context.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        context.recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderState::createVertexBufferStaged() {
    VkDeviceSize bufferSize = sizeof(uint64_t) * 1000 * scene::chunkMap.size();
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    };

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
        &sceneBuffer, &sceneBufferAllocation, nullptr);
   // std::cout << "Updating vertex buffer..." << std::endl;

    // Staging buffer (CPU -> visible)
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = bufferSize;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stagingResultInfo;
    vmaCreateBuffer(allocator, &stagingInfo, &stagingAllocInfo,
        &stagingBuffer, &stagingAllocation, &stagingResultInfo);

    for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
        for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; z++) {
            if (scene::chunkMap.contains(glm::ivec3(x, 0, z))) {
                const auto& data = scene::chunkMeshMap[glm::ivec3(x, 0, z)];
				//std::cout << "Copying data for chunk at position: (" << x << ", 0, " << z << ") to index : " << data.second << std::endl;
                memcpy((char*)stagingResultInfo.pMappedData + sizeof(uint64_t) * 1000 * data.second, data.first.vertices->data(), sizeof(uint64_t) * 1000);
            }
        }
    }
    //std::cout << "Memory copied to staging buffer" << std::endl;
    VkCommandBufferAllocateInfo cbAllocInfo{};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = commandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(context.device, &cbAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, sceneBuffer, 1, &copyRegion);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue); // à remplacer par un fence en prod

    // 4. Nettoyage
    vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);

    //std::cout << "Buffer updated" << std::endl;
}

void RenderState::updateBuffer(glm::ivec3 chunkPos) {
    std::cout << "Updating vertex buffer..." << std::endl;
    VkDeviceSize bufferSize = sizeof(uint64_t) * 1000;

    // 1. Staging buffer (CPU -> visible)
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = bufferSize;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stagingResultInfo;
    vmaCreateBuffer(allocator, &stagingInfo, &stagingAllocInfo,
        &stagingBuffer, &stagingAllocation, &stagingResultInfo);

    const auto& data = scene::chunkMeshMap[chunkPos];
    //std::cout << "Updating chunk at index " << scene::chunkMeshMap[chunkPos].second << ": (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;
    memcpy(stagingResultInfo.pMappedData, data.first.vertices->data(), bufferSize);

    //std::cout << "Memory copied to staging buffer" << std::endl;
    VkCommandBufferAllocateInfo cbAllocInfo{};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = commandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(context.device, &cbAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = sizeof(uint64_t) * 1000 * data.second;
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, sceneBuffer, 1, &copyRegion);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue); // à remplacer par un fence en prod

    // 4. Nettoyage
    vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);

    //std::cout << "Vertex buffer updated for chunk at position: (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;
}

void RenderState::updateIndirectBuffer(glm::ivec3 chunkPos) {
    VkDeviceSize bufferSize = sizeof(VkDrawIndirectCommand) * 6;

    // 1. Staging buffer (CPU -> visible)
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = bufferSize;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stagingResultInfo;
    vmaCreateBuffer(allocator, &stagingInfo, &stagingAllocInfo,
        &stagingBuffer, &stagingAllocation, &stagingResultInfo);

    const uint32_t slot = scene::chunkMeshMap[chunkPos].second;
    memcpy(stagingResultInfo.pMappedData, &commands[6 * slot], (size_t)bufferSize);
    vmaFlushAllocation(
        allocator,
        stagingAllocation,
        0,
        bufferSize
    );

    // 3. Copie staging -> final via une command buffer "one-shot"
    VkCommandBufferAllocateInfo cbAllocInfo{};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = commandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(context.device, &cbAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copyRegions{
        .srcOffset = 0,
        .dstOffset = bufferSize * scene::chunkMeshMap[chunkPos].second,
        .size = bufferSize
    };

    vkCmdCopyBuffer(cmd, stagingBuffer, indirectBuffer, 1, &copyRegions);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue);

    // 4. Nettoyage
    vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

// ── Utilities ─────────────────────────────────────────────────────────────────

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

uint32_t RenderState::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(context.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

RenderState::RenderState() {
    std::vector<std::string> files = {
        "Rock032_4K-JPG_Color",
        "GroundDirtWeedsPatchy004_COL_4K"
    };
    createCommandPool();
    createCommandBuffers();
    createVertexBufferStaged();
    createSyncObjects();
    createIndirectBuffer();
    setupBufferDescriptor();
    createTextures(files, commandPool);
    createGraphicsPipeline();
    createComputePipeline();
}

void RenderState::setupBufferDescriptor() {
    createDescriptorPool();
    createDescriptorSetLayout();
    cameraUniformBuffer.create(allocator);
    createDescriptorSets();
}

void RenderState::createDescriptorSetLayout() {
    auto bindings = {
        VkDescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT
        },
        VkDescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        }
    };

    VkDescriptorSetLayoutCreateInfo descLayoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    if (vkCreateDescriptorSetLayout(context.device, &descLayoutCI, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void RenderState::createIndirectBuffer() {
    commands.resize(6 * scene::chunkMap.size());
    VkDeviceSize bufferSize = sizeof(VkDrawIndirectCommand) * commands.size(); // 6 draw calls par chunks

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo,
        &indirectBuffer, &indirectBufferAlloc, nullptr);

    // Fill the buffer with draw commands
    //std::cout << "Size :" << 6 * scene::chunkMap.size() << std::endl;
    for (int x = -RENDER_DISTANCE; x <= RENDER_DISTANCE; x++) {
        for (int z = -RENDER_DISTANCE; z <= RENDER_DISTANCE; z++) {
            if (!scene::chunkMap.contains(glm::ivec3(x, 0, z))) continue;
            const auto& data = scene::chunkMeshMap[glm::ivec3(x, 0, z)].first;
            const uint32_t globalQuadOffset = 1000 * scene::chunkMeshMap[glm::ivec3(x, 0, z)].second;
            //std::cout << "Iterate for chunk at position: (" << x << ", " << 0 << ", " << z << ") to index " << scene::chunkMap[glm::ivec3(x, 0, z)].second << std::endl;
            for (int i = 0; i < 6; i++) {
                //std::cout << "Command " << scene::chunkMap[glm::ivec3(x, 0, z)].second * 6 + i << std::endl;
                commands[scene::chunkMeshMap[glm::ivec3(x, 0, z)].second * 6 + i] = VkDrawIndirectCommand{
                    .vertexCount = 6,
                    .instanceCount = (unsigned)data.faceVertexLength[i],
                    .firstVertex = (unsigned)(
                        (i & 0b111) << 27 |
                        (x & 0x1FF) << 18 |
                        (0 & 0x1FF) << 9 |
                        (z & 0x1FF)
                    ),
                    .firstInstance = globalQuadOffset + (unsigned)data.faceVertexBegin[i]
                };
            }
        }
    }
    //std::cout << commands.size() << " draw commands generated for " << scene::chunkMap.size() << "chunks" << std::endl;

    // 1. Staging buffer (CPU -> visible)
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = bufferSize;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo stagingAllocInfo{};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo stagingResultInfo;
    vmaCreateBuffer(allocator, &stagingInfo, &stagingAllocInfo,
        &stagingBuffer, &stagingAllocation, &stagingResultInfo);

    memcpy(stagingResultInfo.pMappedData, commands.data(), (size_t)bufferSize);
    vmaFlushAllocation(
        allocator,
        stagingAllocation,
        0,
        bufferSize
    );

    // 3. Copie staging -> final via une command buffer "one-shot"
    VkCommandBufferAllocateInfo cbAllocInfo{};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = commandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(context.device, &cbAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, indirectBuffer, 1, &copyRegion);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(context.graphicsQueue); // à remplacer par un fence en prod

    // 4. Nettoyage
    vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

void RenderState::createDescriptorPool() {
    VkDescriptorPoolSize poolSize[2] = {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1
        }
    };

    VkDescriptorPoolCreateInfo descPoolCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 2,
        .pPoolSizes = poolSize
    };

    if (vkCreateDescriptorPool(context.device, &descPoolCI, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void RenderState::moveScene() {
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(context.device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(cmd, &beginInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDispatch(cmd, (uint32_t)scene::chunkMap.size() * 6, 1, 1);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd
    };

    VkFence fence;
    VkFenceCreateInfo fenceInfo{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    vkCreateFence(context.device, &fenceInfo, nullptr, &fence);

    vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, fence);
    vkWaitForFences(context.device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(context.device, fence, nullptr);
    vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
}

void RenderState::createDescriptorSets() {
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout
    };

    if (vkAllocateDescriptorSets(context.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    VkDescriptorBufferInfo bufferInfo[2]{
        {
            .buffer = cameraUniformBuffer.getBuffer(),
            .offset = 0,
            .range = cameraUniformBuffer.getSize()
        },
        {
            .buffer = indirectBuffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE
        },
    };

    VkWriteDescriptorSet descriptorWrite[2]{
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &bufferInfo[0]
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &bufferInfo[1]
        }
    };

    vkUpdateDescriptorSets(context.device, 2, descriptorWrite, 0, nullptr);
}

void RenderState::update() {
    cameraUniformBuffer.update(camera, projection);
    if (camera.position.x > CHUNK_AXIS1_SIZE || camera.position.x < 0 ||
        camera.position.z > CHUNK_AXIS1_SIZE || camera.position.z < 0) {

        moveScene();

        if (camera.position.x > CHUNK_AXIS1_SIZE) { camera.position.x -= CHUNK_AXIS1_SIZE; scene::centralChunk.x += 1; }
        else if (camera.position.x < 0) { camera.position.x += CHUNK_AXIS1_SIZE; scene::centralChunk.x -= 1; }
        if (camera.position.z > CHUNK_AXIS1_SIZE) { camera.position.z -= CHUNK_AXIS1_SIZE; scene::centralChunk.z += 1; }
        else if (camera.position.z < 0) { camera.position.z += CHUNK_AXIS1_SIZE; scene::centralChunk.z -= 1; }

        scene::notifyMoved();
    }
    glm::ivec3 pos(0);
    auto l = scene::getReadyChunk(pos);
    if (l) {
        updateChunk(pos, true);
    }
}

void RenderState::updateChunk(glm::ivec3 chunkPos, bool includePosition) {
    if (!scene::chunkMap.contains(chunkPos)) throw std::runtime_error("chunk not existe");
    const auto& meshData = scene::chunkMeshMap[chunkPos].first; // const& : évite la copie du MeshData
    const size_t slot = scene::chunkMeshMap[chunkPos].second;

    for (int i = 0; i < 6; i++) {
        const uint32_t globalQuadOffset = 1000 * (uint32_t)slot;
        uint32_t firstVertex = commands[slot * 6 + i].firstVertex; // conservé par défaut

        if (includePosition) {
            glm::ivec3 rel = -chunkPos; // position relative au chunk courant du joueur
            firstVertex = ((uint32_t)(i & 0b111) << 27)
                | ((uint32_t)(rel.x & 0x1FF) << 18)
                | ((uint32_t)(0 & 0x1FF) << 9)
                | (uint32_t)(rel.z & 0x1FF);
        }

        commands[slot * 6 + i] = VkDrawIndirectCommand{
            .vertexCount = 6,
            .instanceCount = (unsigned)meshData.faceVertexLength[i],
            .firstVertex = firstVertex,
            .firstInstance = globalQuadOffset + (unsigned)meshData.faceVertexBegin[i]
        };
    }
    updateBuffer(chunkPos);
    updateIndirectBuffer(chunkPos);
}

RenderState::~RenderState() {
    vkDestroyPipeline(context.device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, pipelineLayout, nullptr);

    vkDestroyPipeline(context.device, computePipeline, nullptr);
    vkDestroyPipelineLayout(context.device, computePipelineLayout, nullptr);

    vkDestroyBuffer(context.device, sceneBuffer, nullptr);
    vmaFreeMemory(allocator, sceneBufferAllocation);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(context.device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(context.device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(context.device, inFlightFences[i], nullptr);
    }
}