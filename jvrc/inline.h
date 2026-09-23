// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "array.h"
#include "config.h"
#include "render.h"
extern inline void jvr_cfg_shape(jvr_config *cfg, VkPrimitiveTopology topology)
{
    cfg->inputAssembly.topology = topology;
}
extern inline void jvr_cfg_bgClearColor(
    jvr_config *cfg, VkClearColorValue clear_color)
{
    cfg->clear_buff[0].color = clear_color;
}
extern inline void jvr_cfg_bgClearDepth(
    jvr_config *cfg, VkClearDepthStencilValue clear_depth)
{
    cfg->clear_buff[1].depthStencil = clear_depth;
}
extern inline void jvr_cfg_add_push_constant(jvr_config *cfg, uint32_t offset,
    uint32_t size, VkShaderStageFlags stageFlags)
{
    VkPushConstantRange push = { stageFlags, offset, size };
    jvr_array_push(&cfg->constRanges, &push);
    cfg->pipelineLayoutInfo.pushConstantRangeCount++;
    cfg->pipelineLayoutInfo.pPushConstantRanges =
        (VkPushConstantRange *)cfg->constRanges.data;
}

extern inline void jvr_cfg_vert_binding(jvr_config *cfg, uint32_t index,
    uint32_t binding, uint32_t stride, VkVertexInputRate input_rate)
{
    cfg->bindingDescription[index].binding = binding;
    cfg->bindingDescription[index].stride = stride;
    cfg->bindingDescription[index].inputRate = input_rate;
}
extern inline void jvr_cfg_vert_attribute(jvr_config *cfg, uint32_t index,
    uint32_t binding, uint32_t location, VkFormat format, uint32_t offset)
{
    cfg->attributeDescription[index].binding = binding;
    cfg->attributeDescription[index].location = location;
    cfg->attributeDescription[index].format = format;
    cfg->attributeDescription[index].offset = offset;
}

extern inline void jvr_render_draw(jvr_renderer *render, uint32_t vertexCount,
    uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(render->m_commandBuffer, vertexCount, instanceCount, firstVertex,
        firstInstance);
}
extern inline void jvr_render_push_constant(jvr_renderer *renderer,
    VkShaderStageFlags stageFlags, const void *data, uint32_t size,
    uint32_t offset)
{
    vkCmdPushConstants(renderer->m_commandBuffer, renderer->m_pipeline_layout,
        stageFlags, offset, size, data);
}

// Turn on validation layers check
// By defualt no validation
extern inline void jvr_render_validate(jvr_renderer *render)
{
    render->setup = (jvr_setup *)calloc(1, sizeof(jvr_setup));
    render->setup->check_validation = 1;
}
extern inline VkResult jvr_check_result(jvr_renderer *renderer)
{
    if (renderer->result != VK_SUCCESS) {
        return renderer->result;
    }
    return renderer->setup->result;
}
extern inline void jvr_map_memory(jvr_renderer *renderer, VkDeviceSize offset,
    VkDeviceSize size, VkMemoryMapFlags flags, void *ppData)
{
    void *data;
    vkMapMemory(renderer->setup->m_logicalDevice,
        renderer->m_vertexBufferMemory, offset, size, flags, &data);
    memcpy(data, ppData, size);
    vkUnmapMemory(
        renderer->setup->m_logicalDevice, renderer->m_vertexBufferMemory);
}

extern inline void jvr_bindVertexBuffers(
    jvr_renderer *renderer, uint32_t firstBinding, uint32_t bindingCount)
{
    VkBuffer vertexBuffers[] = { renderer->m_vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(renderer->m_commandBuffer, firstBinding,
        bindingCount, vertexBuffers, offsets);
}

extern inline void jvr_push_constant(jvr_renderer *renderer,
    VkShaderStageFlags stageFlags, const void *data, uint32_t size,
    uint32_t offset)
{
    vkCmdPushConstants(renderer->m_commandBuffer, renderer->m_pipeline_layout,
        stageFlags, offset, size, data);
}
