// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "render.h"
#include "array.h"
#include "init.h"
#include <vulkan/vulkan_core.h>

void jvr_createVertexBuffer(jvr_renderer *this, uint32_t buffer_size)
{
    VkBufferCreateInfo bufferInfo = { 0 };
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buffer_size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    this->result = vkCreateBuffer(this->setup->m_logicalDevice, &bufferInfo,
        VK_NULL_HANDLE, &this->m_vertexBuffer);
    if (this->result != VK_SUCCESS) {
        THROW("failed to create vertex buffer!", this->result);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(
        this->setup->m_logicalDevice, this->m_vertexBuffer, &memRequirements);
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(
        this->setup->m_gpuDevice, &memProperties);

    VkMemoryAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = jvr_render_findMemoryType(this,
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    this->result = vkAllocateMemory(this->setup->m_logicalDevice, &allocInfo,
        VK_NULL_HANDLE, &this->m_vertexBufferMemory);

    if (this->result != VK_SUCCESS) {
        THROW("failed to allocate vertex buffer memory!", this->result);
    }
    vkBindBufferMemory(this->setup->m_logicalDevice, this->m_vertexBuffer,
        this->m_vertexBufferMemory, 0);
}

void jvr_render_createRenderPass(jvr_renderer *);
void jvr_render_createCommandBuffer(jvr_renderer *);
void jvr_render_recordCommandBuffer(jvr_renderer *this, uint32_t imageIndex);
void jvr_render_createSwapChain(jvr_renderer *);
void jvr_render_createImageViews(jvr_renderer *);
void jvr_render_createDepthViews(jvr_renderer *);
void jvr_render_createSyncObjects(jvr_renderer *);
void jvr_render_drawFrame(jvr_renderer *this, uint32_t imageIndex);

void jvr_render_init(jvr_renderer *this, jvr_config *config,
    const char *app_title, int width, int height)
{
    this->config = config;
    if (this->setup == NULL)
        this->setup = calloc(1, sizeof(jvr_setup));
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        THROW("Can't initialize GLFW", this->result);
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    this->setup->m_window = glfwCreateWindow(
        width, height, app_title, VK_NULL_HANDLE, VK_NULL_HANDLE);
    if (this->setup->m_window == NULL) {
        THROW("Can't create GLFW window", this->result);
    }
    jvr_setup_new(this->setup);
    jvr_setup_init(this->setup);
    jvr_render_createSwapChain(this);
    jvr_render_createImageViews(this);
    jvr_render_createDepthViews(this);
    jvr_render_createRenderPass(this);
}

uint32_t jvr_render_findMemoryType(jvr_renderer *renderer, uint32_t typeFilter,
    VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(
        renderer->setup->m_gpuDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    fprintf(stderr, "failed to find suitable memory type!");
    return 0;
}

void jvr_renderer_render(jvr_renderer *this, void (*func)(jvr_renderer *))
{
    jvr_render_createCommandBuffer(this);
    jvr_render_createSyncObjects(this);
    while (!glfwWindowShouldClose(this->setup->m_window)) {
        glfwPollEvents();
        uint32_t imageIndex;
        this->result = vkWaitForFences(this->setup->m_logicalDevice, 1,
            &this->inFlightFence, VK_TRUE, UINT64_MAX);
        this->result = vkResetFences(
            this->setup->m_logicalDevice, 1, &this->inFlightFence);

        this->result = vkAcquireNextImageKHR(this->setup->m_logicalDevice,
            this->m_swapChain, UINT64_MAX, this->imageAvailableSemaphore,
            VK_NULL_HANDLE, &imageIndex);

        this->result = vkResetCommandBuffer(this->m_commandBuffer, 0);
        jvr_render_recordCommandBuffer(this, imageIndex);
        func(this);

        jvr_render_drawFrame(this, imageIndex);
        usleep(this->config->frame_sleep);
    }
}

void jvr_render_destroy(jvr_renderer *this)
{
    vkDeviceWaitIdle(this->setup->m_logicalDevice);
    vkDestroySemaphore(this->setup->m_logicalDevice,
        this->imageAvailableSemaphore, VK_NULL_HANDLE);
    vkDestroySemaphore(this->setup->m_logicalDevice,
        this->renderFinishedSemaphore, VK_NULL_HANDLE);
    vkDestroyFence(
        this->setup->m_logicalDevice, this->inFlightFence, VK_NULL_HANDLE);

    vkDestroyCommandPool(
        this->setup->m_logicalDevice, this->m_commandPool, VK_NULL_HANDLE);
    FOREACH(this->swapChainFramebuffers, VkFramebuffer, framebuffer)
    {
        vkDestroyFramebuffer(
            this->setup->m_logicalDevice, *framebuffer, VK_NULL_HANDLE);
    }
    vkDestroyPipeline(
        this->setup->m_logicalDevice, this->m_pipeline, VK_NULL_HANDLE);
    vkDestroyPipelineLayout(
        this->setup->m_logicalDevice, this->m_pipeline_layout, VK_NULL_HANDLE);
    vkDestroyRenderPass(
        this->setup->m_logicalDevice, this->m_render_pass, VK_NULL_HANDLE);
    if (this->setup->check_validation) {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                this->setup->m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != VK_NULL_HANDLE) {
            func(this->setup->m_instance, this->setup->debugMessenger,
                VK_NULL_HANDLE);
        }
    }
    FOREACH(this->color_chain.m_images_views, VkImageView, imageView)
    {
        vkDestroyImageView(
            this->setup->m_logicalDevice, *imageView, VK_NULL_HANDLE);
    }
    vkDestroySwapchainKHR(
        this->setup->m_logicalDevice, this->m_swapChain, VK_NULL_HANDLE);
    if (this->m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(
            this->setup->m_logicalDevice, this->m_vertexBuffer, VK_NULL_HANDLE);
        vkFreeMemory(this->setup->m_logicalDevice, this->m_vertexBufferMemory,
            VK_NULL_HANDLE);
        this->m_vertexBuffer = VK_NULL_HANDLE;
        this->m_vertexBufferMemory = VK_NULL_HANDLE;
    }
    for (size_t i = 0; i < this->depth_chain.m_images_views.len; i++) {
        vkDestroyImageView(this->setup->m_logicalDevice,
            JVR_ELEM(this->depth_chain.m_images_views, VkImageView, i),
            VK_NULL_HANDLE);
        vkDestroyImage(this->setup->m_logicalDevice,
            JVR_ELEM(this->depth_chain.m_images, VkImage, i), VK_NULL_HANDLE);
        vkFreeMemory(this->setup->m_logicalDevice,
            JVR_ELEM(this->m_depth_memories, VkDeviceMemory, i),
            VK_NULL_HANDLE);
    }
    free(this->config);
    vkDestroyDevice(this->setup->m_logicalDevice, VK_NULL_HANDLE);
    vkDestroySurfaceKHR(
        this->setup->m_instance, this->setup->m_surface, VK_NULL_HANDLE);
    vkDestroyInstance(this->setup->m_instance, VK_NULL_HANDLE);
    glfwDestroyWindow(this->setup->m_window);
    glfwTerminate();
    free(this->setup);
}
