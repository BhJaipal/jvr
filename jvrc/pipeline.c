// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "array.h"
#include "render.h"
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <vulkan/vulkan_core.h>

jvr_array readFile(const char *filename)
{
    // Open file at the end to read its size, and in binary mode
    FILE *file = fopen(filename, "rb");

    if (!file) {
        THROW("failed to open file: ", 0);
    }

    // Since SPIR-V is made of 32-bit words, resize our vector to fit codeSize / 4
    struct stat info;
    stat(filename, &info);
    size_t fileSize = info.st_size;
    jvr_array buffer;
    jvr_array_new(&buffer, sizeof(uint32_t),
        (fileSize + sizeof(uint32_t) - 1) / sizeof(uint32_t));
    buffer.len = ceil(fileSize * 1.0 / 4);

    fseek(file, 0, SEEK_SET);
    fread(buffer.data, 1, fileSize, file);
    fclose(file);

    return buffer;
}

VkShaderModule loadShader(
    const jvr_array *code, VkDevice device, VkResult *result)
{
    VkShaderModuleCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code->len * sizeof(uint32_t); // Total size in bytes
    createInfo.pCode = code->data; // Already a const uint32_t*

    VkShaderModule shaderModule;
    *result = vkCreateShaderModule(
        device, &createInfo, VK_NULL_HANDLE, &shaderModule);
    if (*result != VK_SUCCESS) {
        THROW("failed to create shader module!", 0);
    }

    return shaderModule;
}
void jvr_render_createFramebuffers(jvr_renderer *);
void jvr_render_createCommandPool(jvr_renderer *);
void jvr_render_createGraphicsPipeline(
    jvr_renderer *this, const char *vertex_shader, const char *fragment_shader)
{
    jvr_array vertSpv = readFile(vertex_shader);
    jvr_array fragSpv = readFile(fragment_shader);

    VkShaderModule vertShader =
        loadShader(&vertSpv, this->setup->m_logicalDevice, &this->result);
    VkShaderModule fragShader =
        loadShader(&fragSpv, this->setup->m_logicalDevice, &this->result);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = { 0 };
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShader;
    vertShaderStageInfo.pName = "main";
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = { 0 };
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShader;
    fragShaderStageInfo.pName = "main";
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo,
    };

    jvr_array dynamicStates = { 0 };
    jvr_array_new(&dynamicStates, sizeof(VkDynamicState), 2);
    JVR_ELEM(dynamicStates, VkDynamicState, 0) = VK_DYNAMIC_STATE_VIEWPORT;
    JVR_ELEM(dynamicStates, VkDynamicState, 1) = VK_DYNAMIC_STATE_SCISSOR;

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)this->m_extent.width;
    viewport.height = (float)this->m_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = { 0 };
    scissor.offset = (VkOffset2D){ 0, 0 };
    scissor.extent = this->m_extent;

    VkPipelineViewportStateCreateInfo viewportState = { 0 };
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineDynamicStateCreateInfo dynamicState = { 0 };
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates.data;

    this->result = vkCreatePipelineLayout(this->setup->m_logicalDevice,
        &this->config->pipelineLayoutInfo, VK_NULL_HANDLE,
        &this->m_pipeline_layout);
    if (this->result != VK_SUCCESS) {
        THROW("failed to create pipeline layout!", this->result);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = { 0 };
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &this->config->vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &this->config->inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &this->config->rasterizer;
    pipelineInfo.pMultisampleState = &this->config->multisampling;
    pipelineInfo.pColorBlendState = &this->config->colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = this->m_pipeline_layout;
    pipelineInfo.renderPass = this->m_render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pDepthStencilState = &this->config->depthStencil;

    this->result = vkCreateGraphicsPipelines(this->setup->m_logicalDevice,
        VK_NULL_HANDLE, 1, &pipelineInfo, VK_NULL_HANDLE, &this->m_pipeline);
    if (this->result != VK_SUCCESS) {
        THROW("failed to create pipeline!", this->result);
    }

    vkDestroyShaderModule(
        this->setup->m_logicalDevice, vertShader, VK_NULL_HANDLE);
    vkDestroyShaderModule(
        this->setup->m_logicalDevice, fragShader, VK_NULL_HANDLE);

    jvr_render_createFramebuffers(this);
    jvr_render_createCommandPool(this);
}

void jvr_render_createRenderPass(jvr_renderer *this)
{
    this->config->attachments[0].format = this->color_chain.m_format;
    this->config->attachments[1].format = this->depth_chain.m_format;

    this->result = vkCreateRenderPass(this->setup->m_logicalDevice,
        &this->config->renderPassInfo, VK_NULL_HANDLE, &this->m_render_pass);
    if (this->result != VK_SUCCESS) {
        THROW("failed to create render pass!", this->result);
    }
}
#define MIN(a, b) a < b ? a : b
void jvr_render_createFramebuffers(jvr_renderer *this)
{
    uint32_t len = MIN(this->color_chain.m_images_views.len,
        this->depth_chain.m_images_views.len);
    jvr_array_new(&this->swapChainFramebuffers, sizeof(VkFramebuffer), len);

    for (size_t i = 0; i < len; i++) {
        VkImageView attachments[] = { JVR_ELEM(this->color_chain.m_images_views,
                                          VkImageView, i),
            JVR_ELEM(this->depth_chain.m_images_views, VkImageView, i) };

        VkFramebufferCreateInfo framebufferInfo = { 0 };
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = this->m_render_pass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = this->m_extent.width;
        framebufferInfo.height = this->m_extent.height;
        framebufferInfo.layers = 1;

        this->result = vkCreateFramebuffer(this->setup->m_logicalDevice,
            &framebufferInfo, VK_NULL_HANDLE,
            JVR_ELEM_PTR(this->swapChainFramebuffers, VkFramebuffer, i));
        if (this->result != VK_SUCCESS) {
            THROW("failed to create framebuffer!", this->result);
        }
    }
}
void jvr_render_createCommandPool(jvr_renderer *this)
{
    uint32_t graphics_index = this->setup->indices >> 32;
    VkCommandPoolCreateInfo poolInfo = { 0 };
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphics_index;
    this->result = vkCreateCommandPool(this->setup->m_logicalDevice, &poolInfo,
        VK_NULL_HANDLE, &this->m_commandPool);
    if (this->result != VK_SUCCESS) {
        THROW("failed to create command pool!", this->result);
    }
}
void jvr_render_createSyncObjects(jvr_renderer *this)
{
    VkSemaphoreCreateInfo semaphoreInfo = { 0 };
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = { 0 };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    this->result = vkCreateSemaphore(this->setup->m_logicalDevice,
        &semaphoreInfo, VK_NULL_HANDLE, &this->imageAvailableSemaphore);
    if (this->result == VK_SUCCESS)
        this->result = vkCreateSemaphore(this->setup->m_logicalDevice,
            &semaphoreInfo, VK_NULL_HANDLE, &this->renderFinishedSemaphore);
    if (this->result == VK_SUCCESS)
        this->result = vkCreateFence(this->setup->m_logicalDevice, &fenceInfo,
            VK_NULL_HANDLE, &this->inFlightFence);

    if (this->result != VK_SUCCESS) {
        THROW("failed to create synchronization objects for a frame!",
            this->result);
    }
}
void jvr_render_createCommandBuffer(jvr_renderer *this)
{
    VkCommandBufferAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    this->result = vkAllocateCommandBuffers(
        this->setup->m_logicalDevice, &allocInfo, &this->m_commandBuffer);
    if (this->result != VK_SUCCESS) {
        THROW("failed to allocate command buffers!", this->result);
    }
}
