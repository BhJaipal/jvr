// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "array.h"
#include "render.h"
#include <vulkan/vulkan_core.h>

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    jvr_array formats;
    jvr_array presentModes;
} SwapChainSupportDetails;

void querySwapChainSupport(SwapChainSupportDetails *details,
    VkPhysicalDevice device, VkSurfaceKHR surface)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device, surface, &details->capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device, surface, &formatCount, VK_NULL_HANDLE);

    if (formatCount != 0) {
        jvr_array_new(
            &details->formats, sizeof(VkSurfaceFormatKHR), formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, surface, &formatCount, details->formats.data);
    }
    details->formats.len = formatCount;

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, VK_NULL_HANDLE);

    if (presentModeCount != 0) {
        jvr_array_new(
            &details->presentModes, sizeof(VkPresentModeKHR), presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &presentModeCount, details->presentModes.data);
        details->presentModes.len = presentModeCount;
    }
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const jvr_array *availableFormats)
{
    FOREACH((*availableFormats), VkSurfaceFormatKHR, availableFormat)
    {
        if (availableFormat->format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return *availableFormat;
        }
    }

    return JVR_ELEM((*availableFormats), VkSurfaceFormatKHR, 0);
}

VkPresentModeKHR chooseSwapPresentMode(const jvr_array availablePresentModes)
{
    FOREACH(availablePresentModes, VkPresentModeKHR, availablePresentMode)
    {
        if (*availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return *availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
#define CLAMP(val, min, max) (val < min) ? (min) : (val > max ? max : val)

VkExtent2D chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR *capabilities, GLFWwindow *window)
{
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = { width, height };

        actualExtent.width = CLAMP(actualExtent.width,
            capabilities->minImageExtent.width,
            capabilities->maxImageExtent.width);
        actualExtent.height = CLAMP(actualExtent.height,
            capabilities->minImageExtent.height,
            capabilities->maxImageExtent.height);

        return actualExtent;
    }
}

VkFormat findDepthFormat(VkPhysicalDevice m_gpuDevice)
{
    VkFormat candidates[] = { VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    for (int i = 0; i < (sizeof candidates / sizeof candidates[0]); i++) {
        VkFormat f = candidates[i];
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_gpuDevice, f, &props);
        if (props.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return f;
    }
    THROW("no supported depth format!", 0);
}
void jvr_render_createSwapChain(jvr_renderer *this)
{
    SwapChainSupportDetails swapChainSupport = { 0 };

    querySwapChainSupport(
        &swapChainSupport, this->setup->m_gpuDevice, this->setup->m_surface);

    VkSurfaceFormatKHR colorSurfaceFormat =
        chooseSwapSurfaceFormat(&swapChainSupport.formats);

    VkPresentModeKHR presentMode =
        chooseSwapPresentMode(swapChainSupport.presentModes);

    VkExtent2D extent =
        chooseSwapExtent(&swapChainSupport.capabilities, this->setup->m_window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = this->setup->m_surface;

    uint32_t present_index = this->setup->indices & UINT32_MAX;
    uint32_t graphics_index = this->setup->indices >> 32;
    uint32_t queueFamilyIndices[] = { graphics_index, present_index };

    if (graphics_index != present_index) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = colorSurfaceFormat.format;
    createInfo.imageColorSpace = colorSurfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;
    this->result = vkCreateSwapchainKHR(this->setup->m_logicalDevice,
        &createInfo, VK_NULL_HANDLE, &this->m_swapChain);
    if (this->result != VK_SUCCESS) {
        THROW("failed to create color swap chain!", this->result);
    }

    vkGetSwapchainImagesKHR(this->setup->m_logicalDevice, this->m_swapChain,
        &imageCount, VK_NULL_HANDLE);
    jvr_array_new(&this->color_chain.m_images, sizeof(VkImage), imageCount);
    vkGetSwapchainImagesKHR(this->setup->m_logicalDevice, this->m_swapChain,
        &imageCount, this->color_chain.m_images.data);
    this->color_chain.m_format = colorSurfaceFormat.format;
    this->m_extent = extent;

    this->depth_chain.m_format = findDepthFormat(this->setup->m_gpuDevice);
}

void jvr_render_createImageViews(jvr_renderer *this)
{
    jvr_array_new(&this->color_chain.m_images_views, sizeof(VkImageView),
        this->color_chain.m_images.len);

    for (size_t i = 0; i < this->color_chain.m_images.len; i++) {
        VkImageViewCreateInfo createInfo = { 0 };
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = JVR_ELEM(this->color_chain.m_images, VkImage, i);
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = this->color_chain.m_format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        this->result = vkCreateImageView(this->setup->m_logicalDevice,
            &createInfo, VK_NULL_HANDLE,
            JVR_ELEM_PTR(this->color_chain.m_images_views, VkImageView, i));
        if (this->result != VK_SUCCESS) {
            THROW("failed to create color image views!", this->result);
        }
    }
}

void jvr_render_createDepthViews(jvr_renderer *this)
{
    VkFormat format = this->depth_chain.m_format;
    uint32_t count = this->color_chain.m_images.len;

    jvr_array_new(&this->m_depth_memories, sizeof(VkDeviceMemory), count);
    jvr_array_new(&this->depth_chain.m_images, sizeof(VkImage), count);
    jvr_array_new(
        &this->depth_chain.m_images_views, sizeof(VkImageView), count);

    char hasStencil = format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                      format == VK_FORMAT_D24_UNORM_S8_UINT ||
                      format == VK_FORMAT_D16_UNORM_S8_UINT;

    for (uint32_t i = 0; i < count; i++) {
        VkImageCreateInfo img = { 0 };
        img.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        img.imageType = VK_IMAGE_TYPE_2D;
        img.format = format;
        img.extent =
            (VkExtent3D){ this->m_extent.width, this->m_extent.height, 1 };
        img.mipLevels = 1;
        img.arrayLayers = 1;
        img.samples = VK_SAMPLE_COUNT_1_BIT;
        img.tiling = VK_IMAGE_TILING_OPTIMAL;
        img.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        img.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        img.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        this->result = vkCreateImage(this->setup->m_logicalDevice, &img,
            VK_NULL_HANDLE,
            JVR_ELEM_PTR(this->depth_chain.m_images, VkImage, i));
        if (this->result != VK_SUCCESS) {
            THROW("failed to create depth image!", this->result);
        }

        VkMemoryRequirements reqs;
        vkGetImageMemoryRequirements(this->setup->m_logicalDevice,
            JVR_ELEM(this->depth_chain.m_images, VkImage, i), &reqs);

        VkMemoryAllocateInfo alloc = { 0 };
        alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc.allocationSize = reqs.size;
        alloc.memoryTypeIndex = jvr_render_findMemoryType(
            this, reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        this->result = vkAllocateMemory(this->setup->m_logicalDevice, &alloc,
            VK_NULL_HANDLE,
            JVR_ELEM_PTR(this->m_depth_memories, VkDeviceMemory, i));
        if (this->result != VK_SUCCESS) {
            THROW("failed to allocate depth memory!", this->result);
        }
        vkBindImageMemory(this->setup->m_logicalDevice,
            JVR_ELEM(this->depth_chain.m_images, VkImage, i),
            JVR_ELEM(this->m_depth_memories, VkDeviceMemory, i), 0);

        VkImageViewCreateInfo view = { 0 };
        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image = JVR_ELEM(this->depth_chain.m_images, VkImage, i);
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (hasStencil)
            view.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.layerCount = 1;

        this->result = vkCreateImageView(this->setup->m_logicalDevice, &view,
            VK_NULL_HANDLE,
            JVR_ELEM_PTR(this->depth_chain.m_images_views, VkImageView, i));
        if (this->result != VK_SUCCESS) {
            THROW("failed to create depth image view!", this->result);
        }
    }
}
