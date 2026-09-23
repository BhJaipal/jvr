// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "render.hpp"
#include <vulkan/vulkan_enums.hpp>

void jvr::renderer::init(
    jvr::config *config, const char *app_title, int width, int height)
{
    this->config = config;
    if (setup == nullptr)
        setup = new jvr::setup;
    setup->init(app_title, width, height);
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createDepthViews();

    config->attachments[0].format = color_chain.m_format;
    config->attachments[1].format = depth_chain.m_format;

    result = m_logicalDevice.createRenderPass(&config->renderPassInfo,
        setup->m_alloca_callback, &m_render_pass, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to create render pass!");
    }
}

void jvr::renderer::createLogicalDevice()
{
    uint32_t present_index = setup->indices.value() & UINT32_MAX;
    uint32_t graphics_index = setup->indices.value() >> 32;
    float queuePriority = 1.0f;

    vk::DeviceQueueCreateInfo *queue = new vk::DeviceQueueCreateInfo[2];
    queue[0].sType = vk::StructureType::eDeviceQueueCreateInfo;
    queue[0].queueFamilyIndex = graphics_index;
    queue[0].queueCount = 1;
    queue[0].pQueuePriorities = &queuePriority;
    queue[0].pNext = nullptr;
    queue[0].flags = vk::DeviceQueueCreateFlags();

    queue[1].sType = vk::StructureType::eDeviceQueueCreateInfo;
    queue[1].queueFamilyIndex = present_index;
    queue[1].queueCount = 1;
    queue[1].pQueuePriorities = &queuePriority;
    queue[1].pNext = nullptr;
    queue[1].flags = vk::DeviceQueueCreateFlags();

    vk::PhysicalDeviceFeatures deviceFeatures;
    vk::DeviceCreateInfo createInfo;

    const char *deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    createInfo.sType = vk::StructureType::eDeviceCreateInfo;
    createInfo.pQueueCreateInfos = queue;
    createInfo.queueCreateInfoCount = graphics_index != present_index ? 2 : 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = 1;

    if (setup->check_validation) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames =
            reinterpret_cast<char **>(&setup->m_validationLayers);
    } else {
        createInfo.enabledLayerCount = 0;
    }
    result = setup->m_gpuDevice.createDevice(&createInfo,
        setup->m_alloca_callback, &m_logicalDevice, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to create logical device!");
    }

    m_logicalDevice.getQueue(
        graphics_index, 0, &m_graphicsQueue, setup->m_dispatch_loader);
    m_logicalDevice.getQueue(
        present_index, 0, &m_presentQueue, setup->m_dispatch_loader);
}

void jvr::renderer::createCommandBuffer()
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;

    result = m_logicalDevice.allocateCommandBuffers(
        &allocInfo, &m_commandBuffer, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to allocate command buffers!");
    }
}

void jvr::renderer::createVertexBuffer(uint32_t buffer_size)
{
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
    bufferInfo.size = buffer_size;
    bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    result = m_logicalDevice.createBuffer(&bufferInfo, setup->m_alloca_callback,
        &m_vertexBuffer, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to create vertex buffer!");
    }

    vk::MemoryRequirements memRequirements;
    m_logicalDevice.getBufferMemoryRequirements(
        m_vertexBuffer, &memRequirements, setup->m_dispatch_loader);
    vk::PhysicalDeviceMemoryProperties memProperties;
    memProperties =
        setup->m_gpuDevice.getMemoryProperties(setup->m_dispatch_loader);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        setup->findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    result = m_logicalDevice.allocateMemory(&allocInfo,
        setup->m_alloca_callback, &m_vertexBufferMemory,
        setup->m_dispatch_loader);

    if (!result) {
        THROW("failed to allocate vertex buffer memory!");
    }
    m_logicalDevice.bindBufferMemory(
        m_vertexBuffer, m_vertexBufferMemory, 0, setup->m_dispatch_loader);
}

void jvr::renderer::destroy()
{
    m_logicalDevice.waitIdle(setup->m_dispatch_loader);
    m_logicalDevice.destroySemaphore(m_imageAvailableSemaphore,
        setup->m_alloca_callback, setup->m_dispatch_loader);
    m_logicalDevice.destroySemaphore(m_renderFinishedSemaphore,
        setup->m_alloca_callback, setup->m_dispatch_loader);
    m_logicalDevice.destroyFence(
        m_inFlightFence, setup->m_alloca_callback, setup->m_dispatch_loader);

    m_logicalDevice.destroyCommandPool(
        m_commandPool, setup->m_alloca_callback, setup->m_dispatch_loader);
    for (auto framebuffer : m_framebuffers) {
        m_logicalDevice.destroyFramebuffer(
            framebuffer, setup->m_alloca_callback, setup->m_dispatch_loader);
    }
    m_logicalDevice.destroyPipeline(
        m_pipeline, setup->m_alloca_callback, setup->m_dispatch_loader);
    m_logicalDevice.destroyPipelineLayout(
        m_pipeline_layout, setup->m_alloca_callback, setup->m_dispatch_loader);
    m_logicalDevice.destroyRenderPass(
        m_render_pass, setup->m_alloca_callback, setup->m_dispatch_loader);

    if (setup->check_validation) {
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)setup->m_instance.getProcAddr(
                "vkDestroyDebugUtilsMessengerEXT", setup->m_dispatch_loader);
        if (func != nullptr) {
            func(setup->m_instance, setup->debugMessenger,
                (VkAllocationCallbacks *)setup->m_alloca_callback);
        }
    }

    for (size_t i = 0; i < depth_chain.m_images_views.size(); i++) {
        m_logicalDevice.destroyImageView(color_chain.m_images_views[i],
            setup->m_alloca_callback, setup->m_dispatch_loader);
        m_logicalDevice.destroyImageView(depth_chain.m_images_views[i],
            setup->m_alloca_callback, setup->m_dispatch_loader);
        m_logicalDevice.destroyImage(depth_chain.m_images[i],
            setup->m_alloca_callback, setup->m_dispatch_loader);
        m_logicalDevice.freeMemory(m_depth_memories[i],
            setup->m_alloca_callback, setup->m_dispatch_loader);
    }

    m_logicalDevice.destroySwapchainKHR(
        m_swapChain, setup->m_alloca_callback, setup->m_dispatch_loader);
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        m_logicalDevice.destroyBuffer(
            m_vertexBuffer, setup->m_alloca_callback, setup->m_dispatch_loader);
        m_logicalDevice.freeMemory(m_vertexBufferMemory,
            setup->m_alloca_callback, setup->m_dispatch_loader);
    }

    m_logicalDevice.destroy(setup->m_alloca_callback, setup->m_dispatch_loader);
    setup->destroy();
    delete config;
    delete setup;
}
