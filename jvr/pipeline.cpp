// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "render.hpp"
#include <cmath>
#include <fstream>
#include <vulkan/vulkan_enums.hpp>

jvr::array<uint32_t> readFile(const std::string &filename)
{
    // Open file at the end to read its size, and in binary mode
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    // Since SPIR-V is made of 32-bit words, resize our vector to fit codeSize / 4
    size_t fileSize = (size_t)file.tellg();
    size_t fileSizeU32 = ceil(fileSize * 1.0 / 4);
    jvr::array<uint32_t> buffer = fileSizeU32;

    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data), fileSize);
    file.close();

    return buffer;
}

vk::ShaderModule loadShader(const jvr::array<uint32_t> &code, vk::Device device,
    vk::Result &result, jvr::setup *setup)
{
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data;

    vk::ShaderModule shaderModule;
    result = device.createShaderModule(&createInfo, setup->m_alloca_callback,
        &shaderModule, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to create shader module!");
    }

    return shaderModule;
}
void jvr::renderer::createGraphicsPipeline(
    const char *vertex_shader, const char *fragment_shader)
{
    auto vertSpv = readFile(vertex_shader);
    auto fragSpv = readFile(fragment_shader);

    vk::ShaderModule vertShader =
        loadShader(vertSpv, m_logicalDevice, result, setup);
    vk::ShaderModule fragShader =
        loadShader(fragSpv, m_logicalDevice, result, setup);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertShader;
    vertShaderStageInfo.pName = "main";
    vertShaderStageInfo.sType =
        vk::StructureType::ePipelineShaderStageCreateInfo;

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragShader;
    fragShaderStageInfo.pName = "main";
    fragShaderStageInfo.sType =
        vk::StructureType::ePipelineShaderStageCreateInfo;

    vk::PipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo,
    };

    vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_extent.width;
    viewport.height = (float)m_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{ 0, 0 };
    scissor.extent = m_extent;

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    result = m_logicalDevice.createPipelineLayout(&config->pipelineLayoutInfo,
        setup->m_alloca_callback, &m_pipeline_layout, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to create pipeline layout!");
    }

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &config->vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &config->inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &config->rasterizer;
    pipelineInfo.pMultisampleState = &config->multisampling;
    pipelineInfo.pColorBlendState = &config->colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipeline_layout;
    pipelineInfo.renderPass = m_render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pDepthStencilState = &config->depthStencil;

    result = m_logicalDevice.createGraphicsPipelines(VK_NULL_HANDLE, 1,
        &pipelineInfo, setup->m_alloca_callback, &m_pipeline,
        setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to create pipeline!");
    }

    m_logicalDevice.destroyShaderModule(
        vertShader, setup->m_alloca_callback, setup->m_dispatch_loader);
    m_logicalDevice.destroyShaderModule(
        fragShader, setup->m_alloca_callback, setup->m_dispatch_loader);

    createFramebuffers();
    createCommandPool();
}

void jvr::renderer::createFramebuffers()
{
    uint32_t len = std::min(
        color_chain.m_images_views.len, depth_chain.m_images_views.len);
    m_framebuffers = len;

    for (size_t i = 0; i < len; i++) {
        vk::ImageView attachments[] = { color_chain.m_images_views[i],
            depth_chain.m_images_views[i] };

        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
        framebufferInfo.renderPass = m_render_pass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_extent.width;
        framebufferInfo.height = m_extent.height;
        framebufferInfo.layers = 1;

        result = m_logicalDevice.createFramebuffer(&framebufferInfo,
            setup->m_alloca_callback, &m_framebuffers[i],
            setup->m_dispatch_loader);
        if (!result) {
            THROW("failed to create framebuffer!");
        }
    }
}
void jvr::renderer::createCommandPool()
{
    uint32_t graphics_index = setup->indices.value() >> 32;
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = graphics_index;
    result = m_logicalDevice.createCommandPool(&poolInfo,
        setup->m_alloca_callback, &m_commandPool, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to create command pool!");
    }
}

void jvr::renderer::createSyncObjects()
{
    vk::SemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    result = m_logicalDevice.createSemaphore(&semaphoreInfo,
        setup->m_alloca_callback, &m_imageAvailableSemaphore,
        setup->m_dispatch_loader);

    if (result == vk::Result::eSuccess)
        result = m_logicalDevice.createSemaphore(&semaphoreInfo,
            setup->m_alloca_callback, &m_renderFinishedSemaphore,
            setup->m_dispatch_loader);

    if (result == vk::Result::eSuccess)
        result = m_logicalDevice.createFence(&fenceInfo,
            setup->m_alloca_callback, &m_inFlightFence,
            setup->m_dispatch_loader);

    if (!result) {
        THROW("failed to create synchronization objects for a frame!");
    }
}
