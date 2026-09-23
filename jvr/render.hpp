// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#ifndef JVR_RENDER_H
#define JVR_RENDER_H
#include "config.hpp"
#include <functional>
#include <cstdint>
#include <unistd.h>

#include "init.hpp"

#define THROW(msg)                                          \
    throw std::runtime_error(msg "\n" __FILE__ ":" +        \
                             std::to_string(__LINE__ - 3) + \
                             "\nCheck this.result to know the error")

inline bool operator!(vk::Result res)
{
    return res != vk::Result::eSuccess;
}

namespace jvr
{

struct image_buffer {
    array<vk::Image> m_images;
    vk::Format m_format;
    array<vk::ImageView> m_images_views;
};

class renderer {
    jvr::setup *setup = nullptr;

    vk::Device m_logicalDevice;
    vk::Queue m_graphicsQueue;
    vk::Queue m_presentQueue;

    vk::SwapchainKHR m_swapChain;
    vk::Extent2D m_extent;
    image_buffer color_chain;
    image_buffer depth_chain;
    jvr::array<vk::DeviceMemory> m_depth_memories;

    vk::RenderPass m_render_pass;
    vk::PipelineLayout m_pipeline_layout;
    vk::Pipeline m_pipeline;
    jvr::array<vk::Framebuffer> m_framebuffers;
    vk::CommandPool m_commandPool;
    vk::Semaphore m_imageAvailableSemaphore;
    vk::Semaphore m_renderFinishedSemaphore;
    vk::Fence m_inFlightFence;

    vk::CommandBuffer m_commandBuffer;
    vk::Buffer m_vertexBuffer;
    vk::DeviceMemory m_vertexBufferMemory;

    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createDepthViews();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffer();
    uint32_t recordCommandBuffer();

    void createSyncObjects();
    void drawFrame(uint32_t imageIndex);

public:
    vk::Result result = vk::Result::eSuccess;
    jvr::config *config;

    void createGraphicsPipeline(
        const char *vertex_shader, const char *fragment_shader);

    void init(jvr::config *config, const char *app_title, int width = 800,
        int height = 600);

    void createVertexBuffer(uint32_t buffer_size);

    inline vk::Result check_result()
    {
        if (!result) {
            return result;
        }
        return setup->result;
    }

    // Turn on validation layers check
    // By defualt no validation
    inline void validate() noexcept
    {
        setup = new jvr::setup;
        setup->check_validation = true;
    }

    void mapMemory(vk::DeviceSize offset, vk::DeviceSize size,
        vk::MemoryMapFlags flags, void *ppData)
    {
        void *data;
        result = m_logicalDevice.mapMemory(m_vertexBufferMemory, offset, size,
            flags, &data, setup->m_dispatch_loader);
        if (result != vk::Result::eSuccess) {
            THROW("Failed to map memory");
        }
        memcpy(data, ppData, size);
        m_logicalDevice.unmapMemory(
            m_vertexBufferMemory, setup->m_dispatch_loader);
    }

    void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount)
    {
        vk::Buffer vertexBuffers[] = { m_vertexBuffer };
        vk::DeviceSize offsets[] = { 0 };
        m_commandBuffer.bindVertexBuffers(firstBinding, bindingCount,
            vertexBuffers, offsets, setup->m_dispatch_loader);
    }

    inline void draw(uint32_t vertexCount, uint32_t instanceCount,
        uint32_t firstVertex = 0, uint32_t firstInstance = 0)
    {
        m_commandBuffer.draw(vertexCount, instanceCount, firstVertex,
            firstInstance, setup->m_dispatch_loader);
    }

    void render(std::function<void(renderer &)> func)
    {
        createCommandBuffer();
        createSyncObjects();
        while (!glfwWindowShouldClose(setup->m_window)) {
            glfwPollEvents();
            uint32_t imageIndex = recordCommandBuffer();
            func(*this);

            drawFrame(imageIndex);
            usleep(config->frame_sleep);
        }
    }

    inline void push_constant(vk::ShaderStageFlags stageFlags, const void *data,
        uint32_t size, uint32_t offset)
    {
        m_commandBuffer.pushConstants(m_pipeline_layout, stageFlags, offset,
            size, data, setup->m_dispatch_loader);
    }

    void destroy();
};
}
#endif // !JVR_RENDER_H
