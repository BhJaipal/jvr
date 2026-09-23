// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#ifndef JVR_RENDER_H
#define JVR_RENDER_H
#include "config.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "init.h"
#include <vulkan/vk_enum_string_helper.h>

#define THROW(msg, res)                                                     \
    printf(msg "\n" __FILE__ ":%d\nCheck this.result to know the error %s", \
        __LINE__ - 3, string_VkResult(res));                                \
    exit(1);

struct image_buffer {
    jvr_array m_images;
    VkFormat m_format;
    jvr_array m_images_views;
};

typedef struct jvr_renderer {
    jvr_setup *setup;
    VkSwapchainKHR m_swapChain;
    VkExtent2D m_extent;
    struct image_buffer color_chain;
    struct image_buffer depth_chain;
    jvr_array m_depth_memories;

    VkRenderPass m_render_pass;
    VkPipelineLayout m_pipeline_layout;
    VkPipeline m_pipeline;
    jvr_array swapChainFramebuffers;
    VkCommandPool m_commandPool;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    VkCommandBuffer m_commandBuffer;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;

    VkResult result;
    jvr_config *config;
} jvr_renderer;

extern void jvr_render_init(jvr_renderer *render, jvr_config *config,
    const char *app_title, int width, int height);

extern uint32_t jvr_render_findMemoryType(jvr_renderer *renderer,
    uint32_t typeFilter, VkMemoryPropertyFlags properties);

extern void jvr_renderer_render(
    jvr_renderer *render, void (*func)(jvr_renderer *));

static void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d %s\n", error, description);
    exit(1);
}

extern void jvr_render_createGraphicsPipeline(jvr_renderer *render,
    const char *vertex_shader, const char *fragment_shader);

extern void jvr_render_destroy(jvr_renderer *render);

extern void jvr_createVertexBuffer(
    jvr_renderer *renderer, uint32_t buffer_size);

#endif // !VK_RENDER_H
