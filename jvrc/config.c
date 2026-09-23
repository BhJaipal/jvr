// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "config.h"
#include <stdlib.h>

void jvr_cfg_new(jvr_config *config)
{
    config->frame_sleep = 16666;
    jvr_array_new(&config->constRanges, sizeof(VkPushConstantRange), 1);
    config->constRanges.len = 0;
    config->clear_buff[0].color = (VkClearColorValue){ { 0.5, 0.9, 0.7, 1.0 } };
    config->clear_buff[1].depthStencil = (VkClearDepthStencilValue){ 1.0, 0 };

    VkAttachmentDescription *colorAttachment = config->attachments;
    colorAttachment->samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    config->colorAttachmentRef.attachment = 0;
    config->colorAttachmentRef.layout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription *depthAttachment = config->attachments + 1;
    depthAttachment->format =
        VK_FORMAT_D32_SFLOAT; // match what you allocate the depth image as
    depthAttachment->samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment->finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    config->depthAttachmentRef.attachment = 1; // index after colorAttachment
    config->depthAttachmentRef.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    config->subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    config->subpass.colorAttachmentCount = 1;
    config->subpass.pColorAttachments = &config->colorAttachmentRef;
    config->subpass.pDepthStencilAttachment = &config->depthAttachmentRef;

    config->renderPassInfo.attachmentCount = 2;
    config->renderPassInfo.pAttachments = config->attachments;

    config->renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    config->renderPassInfo.subpassCount = 1;
    config->renderPassInfo.pSubpasses = &config->subpass;

    config->rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    config->rasterizer.depthClampEnable = VK_FALSE;
    config->rasterizer.rasterizerDiscardEnable = VK_FALSE;
    config->rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    config->rasterizer.lineWidth = 1.0f;
    config->rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    config->rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    config->rasterizer.depthBiasEnable = VK_TRUE;
    config->rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    config->rasterizer.depthBiasClamp = 0.0f; // Optional
    config->rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    config->multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    config->multisampling.sampleShadingEnable = VK_FALSE;
    config->multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    config->multisampling.minSampleShading = 1.0f; // Optional
    config->multisampling.pSampleMask = VK_NULL_HANDLE; // Optional
    config->multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    config->multisampling.alphaToOneEnable = VK_FALSE; // Optional

    config->colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    config->colorBlendAttachment.blendEnable = VK_TRUE;
    config->colorBlendAttachment.srcColorBlendFactor =
        VK_BLEND_FACTOR_SRC_ALPHA;
    config->colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    config->colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    config->colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    config->colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    config->colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    config->colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    config->colorBlending.logicOpEnable = VK_FALSE;
    config->colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    config->colorBlending.attachmentCount = 1;
    config->colorBlending.pAttachments = &config->colorBlendAttachment;
    config->colorBlending.blendConstants[0] = 0.0f; // Optional
    config->colorBlending.blendConstants[1] = 0.0f; // Optional
    config->colorBlending.blendConstants[2] = 0.0f; // Optional
    config->colorBlending.blendConstants[3] = 0.0f;

    config->pipelineLayoutInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    config->pipelineLayoutInfo.setLayoutCount = 0;
    config->pipelineLayoutInfo.pushConstantRangeCount = 0;
    config->pipelineLayoutInfo.pPushConstantRanges = config->constRanges.data;

    config->inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    config->inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    config->inputAssembly.primitiveRestartEnable = VK_FALSE;

    config->depthStencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    config->depthStencil.depthTestEnable = VK_TRUE;
    config->depthStencil.depthWriteEnable = VK_TRUE;
    config->depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    config->depthStencil.depthBoundsTestEnable = VK_FALSE;
    config->depthStencil.stencilTestEnable = VK_FALSE;
}

void jvr_cfg_vertex_input_props(
    jvr_config *cfg, uint32_t binding_length, uint32_t attribute_len)
{
    if (binding_length != 0)
        cfg->bindingDescription =
            malloc(sizeof(VkVertexInputBindingDescription) * binding_length);
    if (attribute_len != 0)
        cfg->attributeDescription =
            malloc(sizeof(VkVertexInputAttributeDescription) * attribute_len);

    cfg->vertexInputInfo.vertexBindingDescriptionCount = binding_length;
    cfg->vertexInputInfo.vertexAttributeDescriptionCount = attribute_len;
    cfg->vertexInputInfo.pVertexBindingDescriptions = cfg->bindingDescription;
    cfg->vertexInputInfo.pVertexAttributeDescriptions =
        cfg->attributeDescription;
    cfg->vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}
