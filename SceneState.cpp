#include "SceneState.h"
#include "texture.h"
#include <fstream>
#include <iostream>
#include "scene.h"
#include "util.hpp"

// ── pipeline ────────────────────────────────────────────────────

void SceneState::createGraphicsPipeline() {
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

    VkPipelineVertexInputStateCreateInfo vertexInputState{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 0,
    .pVertexBindingDescriptions = nullptr,
    .vertexAttributeDescriptionCount = 0,
    .pVertexAttributeDescriptions = nullptr
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
        cameraDescriptorLayout,
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

    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingCI,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
    };

    if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(context.device, shaderModule, nullptr);
}

void SceneState::createComputePipeline() {
    auto shaderCode = readFile("shaders/build/scene_update.spv");

    VkShaderModule shaderModule = createShaderModule(shaderCode);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = shaderModule;
    computeShaderStageInfo.pName = "move_scene";

    auto layouts = {
        cameraDescriptorLayout,
        descriptorSetLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = (uint32_t)layouts.size(),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 0
    };

    if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &sceneMovePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline layout!");
    }

    VkComputePipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = computeShaderStageInfo,
        .layout = sceneMovePipelineLayout,
    };

    if (vkCreateComputePipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &sceneMovePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }

    vkDestroyShaderModule(context.device, shaderModule, nullptr);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    shaderCode = readFile("shaders/build/frustrum.spv");

    shaderModule = createShaderModule(shaderCode);

    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = shaderModule;
    computeShaderStageInfo.pName = "frustum_culling";

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (uint32_t)layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &frustrumPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create frustrum pipeline layout!");
    }

    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = computeShaderStageInfo;
    pipelineInfo.layout = frustrumPipelineLayout;

    if (vkCreateComputePipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &frustrumPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create frustrum pipeline!");
    }

    vkDestroyShaderModule(context.device, shaderModule, nullptr);
}

// ── commands ───────────────────────────────────────────────────

void SceneState::recordCommandBuffer(VkCommandBuffer& cmdBuffer) {
    auto set = { cameraDescriptor, descriptorSet, getDescriptorSetTexture() };
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, set.size(), set.data(), 0, nullptr);
    vkCmdDrawIndirect(cmdBuffer, indirectBuffer, 0, 6 * (uint32_t)scene::chunkMap.size(), sizeof(VkDrawIndirectCommand));
}

void SceneState::recordCommandFrustrum(VkCommandBuffer& cmdBuffer) {
    // --- BARRIÈRE : Attendre que indirectBuffer soit prêt ---
    VkBufferMemoryBarrier2 indirectBufferBarrier{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,       // Stage source : vkCmdCopyBuffer (dans updateIndirectBuffer)
        .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,        // Accès source : écriture par transfert
        .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, // Stage destination : frustum culling
        .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,           // Accès destination : lecture par le compute shader
        .buffer = indirectBuffer,
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };

    VkDependencyInfo depInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &indirectBufferBarrier
    };
    vkCmdPipelineBarrier2(cmdBuffer, &depInfo);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, frustrumPipeline);
    auto layout = {
        cameraDescriptor,
        descriptorSet
    };
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, frustrumPipelineLayout, 0, layout.size(), layout.data(), 0, nullptr);
    vkCmdDispatch(cmdBuffer, (uint32_t)scene::chunkMap.size(), 1, 1);

    VkMemoryBarrier2 frustrumBarrier{
    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
    .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, // Stage source : compute shader
    .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,          // Accès source : écriture par le compute shader
    .dstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,  // Stage destination : draw indirect
    .dstAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT  // Accès destination : lecture des commandes indirectes
    };
    VkDependencyInfo frustrumDepInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &frustrumBarrier
    };
    vkCmdPipelineBarrier2(cmdBuffer, &frustrumDepInfo);

}

void SceneState::createVertexBufferStaged() {
    size_t chunkCount = scene::chunkMeshMap.size();
    sceneBuffer.resize(chunkCount);
    sceneBufferAllocation.resize(chunkCount);

    VkDeviceSize bufferSize = sizeof(uint64_t) * MAX_FACE;
    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    };

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    for (size_t i = 0; i < chunkCount; ++i) {
        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &sceneBuffer[i], &sceneBufferAllocation[i], nullptr);
    }

    // Staging buffer (CPU -> visible), réutilisé pour chaque chunk
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAllocation = VK_NULL_HANDLE;

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

    // Command buffer alloué une seule fois, réutilisé (reset) à chaque chunk
    VkCommandBufferAllocateInfo cbAllocInfo{};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = commandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(context.device, &cbAllocInfo, &cmd);

    for (const auto& [key, mesh] : scene::chunkMeshMap) {
        memcpy(stagingResultInfo.pMappedData, mesh.vertices->data(), bufferSize);

        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkBufferCopy copyRegion{ .size = bufferSize };
        vkCmdCopyBuffer(cmd, stagingBuffer, sceneBuffer[index_of(scene::chunkMeshMap, key)], 1, &copyRegion);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(context.graphicsQueue); // à remplacer par un fence en prod
    }

    // Nettoyage
    vkFreeCommandBuffers(context.device, commandPool, 1, &cmd);
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
}

void SceneState::updateBuffer(glm::ivec3 chunkPos) {
    //std::cout << "Updating vertex buffer..." << std::endl;
    VkDeviceSize bufferSize = sizeof(uint64_t) * MAX_FACE;

    // 1. Staging buffer (CPU -> visible)
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAllocation = VK_NULL_HANDLE;

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

    auto slot = index_of(scene::chunkMeshMap, chunkPos);
    //std::cout << "Updating chunk at index " << slot << ": (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;
    const auto& data = scene::chunkMeshMap[chunkPos];
    //std::cout << "Updating chunk at index " << scene::chunkMeshMap[chunkPos].second << ": (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;
    memcpy(stagingResultInfo.pMappedData, data.vertices->data(), bufferSize);

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

    VkBufferCopy copyRegion{
        .size = bufferSize
    };
    vkCmdCopyBuffer(cmd, stagingBuffer, sceneBuffer[slot], 1, &copyRegion);

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

void SceneState::updateIndirectBuffer(glm::ivec3 chunkPos) {
    //std::cout << "Updating indirect buffer..." << std::endl;
    VkDeviceSize bufferSize = sizeof(VkDrawIndirectCommand) * 6;


    // 1. Staging buffer (CPU -> visible)
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAllocation = VK_NULL_HANDLE;

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

    // Le STAGING ne contient qu'UN SEUL slot (6 commandes) -> écrit à
    // l'offset 0. C'est copyRegion.dstOffset, plus bas, qui place ces
    // données au bon slot dans indirectBuffer.
    auto slot = index_of(scene::chunkMeshMap, chunkPos);
    //std::cout << "Updating chunk at index " << slot << ": (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;
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

    std::vector<VkBufferCopy> copyRegions;
    copyRegions.reserve(12); // 2 régions * 6 commandes

    for (int i = 0; i < 6; i++) {
        VkDeviceSize base = i * sizeof(VkDrawIndirectCommand);
        VkDeviceSize dstBase = (6 * slot + i) * sizeof(VkDrawIndirectCommand);

        // vertexCount + instanceCount (8 premiers octets)
        copyRegions.push_back({ base + 0, dstBase + 0, 8 });
        // firstInstance seulement (on saute firstVertex, offset 8-12)
        copyRegions.push_back({ base + 12, dstBase + 12, 4 });
    }

    vkCmdCopyBuffer(cmd, stagingBuffer, indirectBuffer,
        (uint32_t)copyRegions.size(), copyRegions.data());

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

    //std::cout << "Indirect buffer updated for chunk at position: (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;
}

SceneState::SceneState(VkDescriptorSet& descriptor, VkDescriptorSetLayout& descriptorLayout) : cameraDescriptor(descriptor), cameraDescriptorLayout(descriptorLayout) {
    createCommandPool();
    createVertexBufferStaged();
    createIndirectBuffer();
    setupBufferDescriptor();
    createGraphicsPipeline();
    createComputePipeline();
}

void SceneState::setupBufferDescriptor() {
    createDescriptorPool();
    createDescriptorSetLayout();
    createDescriptorSets();
}

void SceneState::createDescriptorSetLayout() {
    auto bindings = {
        VkDescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
        },
        VkDescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = (uint32_t)scene::chunkMeshMap.size(),
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT
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

void SceneState::createIndirectBuffer() {
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
    for (const auto& [chunkPos, data] : scene::chunkMeshMap) {
        auto slot = index_of(scene::chunkMeshMap, chunkPos);
        std::cout << "Iterate for chunk at position: (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ") to index " << slot << std::endl;
        for (int i = 0; i < 6; i++) {
            std::cout << "Command " << slot * 6 + i << std::endl;
            commands[slot * 6 + i] = VkDrawIndirectCommand{
                .vertexCount = 6,
                .instanceCount = (unsigned)data.faceVertexLength[i],
                .firstVertex = (unsigned)(
                    (i & 0b111) << 24 |
                    (chunkPos.x & 0xFF) << 16 |
                    (chunkPos.y & 0xFF) << 8 |
                    (chunkPos.z & 0xFF)
                ),
                .firstInstance = (unsigned)data.faceVertexBegin[i]
            };
        }
    }
    std::cout << commands.size() << " draw commands generated for " << scene::chunkMap.size() << "chunks" << std::endl;

    // 1. Staging buffer (CPU -> visible)
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAllocation = VK_NULL_HANDLE;

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

void SceneState::moveScene() {
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

    auto set = {
        cameraDescriptor,
        descriptorSet
    };

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, sceneMovePipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, sceneMovePipelineLayout, 0, set.size(), set.data(), 0, nullptr);
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

void SceneState::createDescriptorSets() {
    VkDescriptorSetAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayout
    };

    if (vkAllocateDescriptorSets(context.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    auto bufferInfo = {
        VkDescriptorBufferInfo {
            .buffer = indirectBuffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE
        }
    };

    std::vector<VkDescriptorBufferInfo> chunksBufferInfos;
    for (auto& chunkBuffer : sceneBuffer) {
        chunksBufferInfos.push_back(VkDescriptorBufferInfo{
            .buffer = chunkBuffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE
            });
    }

    auto descriptorWrite = {
        VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = bufferInfo.data()
        },
        VkWriteDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = (uint32_t)scene::chunkMeshMap.size(),
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = chunksBufferInfos.data()
        }
    };

    vkUpdateDescriptorSets(context.device, descriptorWrite.size(), descriptorWrite.data(), 0, nullptr);
}

void SceneState::update() {
    if (camera.position.x > CHUNK_AXIS1_SIZE || camera.position.x < 0 ||
        camera.position.z > CHUNK_AXIS1_SIZE || camera.position.z < 0) {

        moveScene();

        if (camera.position.x > CHUNK_AXIS1_SIZE) scene::centralChunk.x += 1;
        else if (camera.position.x < 0) scene::centralChunk.x -= 1;
        if (camera.position.z > CHUNK_AXIS1_SIZE) scene::centralChunk.z += 1;
        else if (camera.position.z < 0) scene::centralChunk.z -= 1;

        scene::notifyMoved();
    }
    glm::ivec3 pos(0);
    auto l = scene::getReadyChunk(pos);
    if (l) {
        updateChunk(pos);
    }
}

void SceneState::createDescriptorPool() {
    auto poolSize = {
        VkDescriptorPoolSize {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = (uint32_t)(1 + scene::chunkMap.size())
        }
    };

    VkDescriptorPoolCreateInfo descPoolCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = (uint32_t)poolSize.size(),
        .pPoolSizes = poolSize.data()
    };

    if (vkCreateDescriptorPool(context.device, &descPoolCI, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}


void SceneState::updateChunk(glm::ivec3 chunkPos) {
    if (!scene::chunkMap.contains(chunkPos)) throw std::runtime_error("chunk not existe");
    const auto& meshData = scene::chunkMeshMap[chunkPos];
    auto slot = index_of(scene::chunkMeshMap, chunkPos);

    for (int i = 0; i < 6; i++) {
        VkDrawIndirectCommand& comd = commands[slot * 6 + i];

        comd.firstInstance = (unsigned)meshData.faceVertexBegin[i];
        comd.instanceCount = (unsigned)meshData.faceVertexLength[i];
    }

    //std::cout << "Updating chunk at position: (" << chunkPos.x << ", " << chunkPos.y << ", " << chunkPos.z << ")" << std::endl;
    updateBuffer(chunkPos);
    updateIndirectBuffer(chunkPos);
}

SceneState::~SceneState() {
    vkDestroyPipeline(context.device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, pipelineLayout, nullptr);

    vkDestroyPipeline(context.device, sceneMovePipeline, nullptr);
    vkDestroyPipelineLayout(context.device, sceneMovePipelineLayout, nullptr);
    vkDestroyPipeline(context.device, frustrumPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, frustrumPipelineLayout, nullptr);

    vkDestroyBuffer(context.device, indirectBuffer, nullptr);
    vmaFreeMemory(allocator, indirectBufferAlloc);

    for (int i = 0; i < sceneBuffer.size(); i++) {
        vkDestroyBuffer(context.device, sceneBuffer[i], nullptr);
        vmaFreeMemory(allocator, sceneBufferAllocation[i]);
    }
}