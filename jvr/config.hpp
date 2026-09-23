// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#ifndef JVR_CONFIG_H
#define JVR_CONFIG_H

#include "array.hpp"
#include <cstring>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
namespace jvr
{
struct config {
    vk::AttachmentDescription attachments[2];
    vk::AttachmentReference depthAttachmentRef{};
    vk::AttachmentReference colorAttachmentRef{};
    vk::SubpassDescription subpass{};
    vk::RenderPassCreateInfo renderPassInfo{};

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    jvr::array<vk::PushConstantRange> constRanges;
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};

    vk::VertexInputBindingDescription *bindingDescription = nullptr;
    vk::VertexInputAttributeDescription *attributeDescription = nullptr;
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

    vk::PipelineDepthStencilStateCreateInfo depthStencil{};

    vk::ClearValue clear_buff[2];

    uint32_t frame_sleep = 16666;

    config()
        : constRanges(1)
    {
        constRanges.len = 0;
        clear_buff[0].color = vk::ClearColorValue{ 0.5f, 0.9, 0.7, 1.0 };
        clear_buff[1].depthStencil = vk::ClearDepthStencilValue{ 1.0, 0 };

        vk::AttachmentDescription &colorAttachment = attachments[0];
        colorAttachment.samples = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        vk::AttachmentDescription &depthAttachment = attachments[1];
        depthAttachment.format = vk::Format::eD32Sfloat;
        depthAttachment.samples = vk::SampleCountFlagBits::e1;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout =
            vk::ImageLayout::eDepthStencilAttachmentOptimal;

        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout =
            vk::ImageLayout::eDepthStencilAttachmentOptimal;

        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = attachments;

        renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        rasterizer.sType =
            vk::StructureType::ePipelineRasterizationStateCreateInfo;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eClockwise;
        rasterizer.depthBiasEnable = VK_TRUE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        multisampling.sType =
            vk::StructureType::ePipelineMultisampleStateCreateInfo;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        colorBlendAttachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachment.dstColorBlendFactor =
            vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

        colorBlending.sType =
            vk::StructureType::ePipelineColorBlendStateCreateInfo;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = vk::LogicOp::eCopy; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f;

        pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = constRanges.data;

        inputAssembly.sType =
            vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        depthStencil.sType =
            vk::StructureType::ePipelineDepthStencilStateCreateInfo;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = vk::CompareOp::eLess;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
    }

    void bgClearColor(vk::ClearColorValue clear_color)
    {
        clear_buff[0].color = clear_color;
    }
    void bgClearDepth(vk::ClearDepthStencilValue clear_depth)
    {
        clear_buff[1].depthStencil = clear_depth;
    }

    void add_push_constant(
        uint32_t offset, uint32_t size, vk::ShaderStageFlags stageFlags)
    {
        constRanges.push(vk::PushConstantRange{ stageFlags, offset, size });
        constRanges[constRanges.len - 1].stageFlags = stageFlags;
        constRanges[constRanges.len - 1].offset = offset;
        constRanges[constRanges.len - 1].size = size;
        pipelineLayoutInfo.pushConstantRangeCount++;
        pipelineLayoutInfo.pPushConstantRanges = constRanges.data;
    }

    void vertex_input_props(uint32_t binding_length, uint32_t attribute_len)
    {
        if (binding_length != 0)
            bindingDescription =
                new vk::VertexInputBindingDescription[binding_length];
        if (attribute_len != 0)
            attributeDescription =
                new vk::VertexInputAttributeDescription[attribute_len];

        vertexInputInfo.vertexBindingDescriptionCount = binding_length;
        vertexInputInfo.vertexAttributeDescriptionCount = attribute_len;
        vertexInputInfo.pVertexBindingDescriptions = bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescription;
        vertexInputInfo.sType =
            vk::StructureType::ePipelineVertexInputStateCreateInfo;
    }
    inline void vert_binding(uint32_t index, uint32_t binding, uint32_t stride,
        vk::VertexInputRate input_rate)
    {
        bindingDescription[index].binding = binding;
        bindingDescription[index].stride = stride;
        bindingDescription[index].inputRate = input_rate;
    }
    inline void vert_attribute(uint32_t index, uint32_t binding,
        uint32_t location, vk::Format format, uint32_t offset)
    {
        attributeDescription[index].binding = binding;
        attributeDescription[index].location = location;
        attributeDescription[index].format = format;
        attributeDescription[index].offset = offset;
    }
    void shape(vk::PrimitiveTopology topology)
    {
        inputAssembly.topology = topology;
    }
};
}

#endif // !JVK_CONFIG_H
