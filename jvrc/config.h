// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#ifndef JVR_CONFIG_H
#define JVR_CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
#include "array.h"

typedef struct jvr_config {
    VkAttachmentDescription attachments[2];
    VkAttachmentReference depthAttachmentRef;
    VkAttachmentReference colorAttachmentRef;
    VkSubpassDescription subpass;
    VkRenderPassCreateInfo renderPassInfo;

    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlending;
    jvr_array constRanges;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;

    VkVertexInputBindingDescription *bindingDescription;
    VkVertexInputAttributeDescription *attributeDescription;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;

    VkPipelineDepthStencilStateCreateInfo depthStencil;

    VkClearValue clear_buff[2];

    uint32_t frame_sleep;
} jvr_config;

extern void jvr_cfg_new(jvr_config *config);
extern void jvr_cfg_vertex_input_props(
    jvr_config *cfg, uint32_t binding_length, uint32_t attribute_len);

#endif // !JVK_CONFIG_H
