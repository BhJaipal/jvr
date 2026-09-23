// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "render.hpp"
#include <vulkan/vulkan_enums.hpp>

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    jvr::array<vk::SurfaceFormatKHR> formats;
    jvr::array<vk::PresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(
    vk::PhysicalDevice device, vk::SurfaceKHR surface, vk::Result &result)
{
    SwapChainSupportDetails details;

    result = device.getSurfaceCapabilitiesKHR(surface, &details.capabilities);
    if (result != vk::Result::eSuccess) {
        THROW("failed to get surface capabilities");
    }

    uint32_t formatCount;
    result = device.getSurfaceFormatsKHR(surface, &formatCount, nullptr);
    if (!result) {
        THROW("failed to get surface format count");
    }

    if (formatCount != 0) {
        details.formats = (formatCount);
        result = device.getSurfaceFormatsKHR(
            surface, &formatCount, details.formats.data);
        if (!result) {
            THROW("failed to get surface formats");
        }
    }

    uint32_t presentModeCount;
    result =
        device.getSurfacePresentModesKHR(surface, &presentModeCount, nullptr);
    if (!result) {
        THROW("failed to get surface present mode count");
    }

    if (presentModeCount != 0) {
        details.presentModes = presentModeCount;
        result = device.getSurfacePresentModesKHR(
            surface, &presentModeCount, details.presentModes.data);
        if (!result) {
            THROW("failied to get surface present mode");
        }
    }

    return details;
}

vk::SurfaceFormatKHR findColorFormat(
    const jvr::array<vk::SurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::Format findDepthFormat(vk::PhysicalDevice m_gpuDevice)
{
    vk::Format candidates[] = { vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };
    for (vk::Format f : candidates) {
        vk::FormatProperties props;
        m_gpuDevice.getFormatProperties(f, &props);
        if (props.optimalTilingFeatures &
            vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            return f;
    }
    THROW("no supported depth format!");
}

vk::PresentModeKHR findPresentMode(
    const jvr::array<vk::PresentModeKHR> &availablePresentModes)
{
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D findSwapExtent(
    const vk::SurfaceCapabilitiesKHR &capabilities, GLFWwindow *window)
{
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        vk::Extent2D actualExtent = { static_cast<uint32_t>(width),
            static_cast<uint32_t>(height) };

        actualExtent.width = std::clamp(actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void jvr::renderer::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(setup->m_gpuDevice, setup->m_surface, result);

    vk::SurfaceFormatKHR colorSurfaceFormat =
        findColorFormat(swapChainSupport.formats);

    vk::PresentModeKHR presentMode =
        findPresentMode(swapChainSupport.presentModes);

    VkExtent2D extent =
        findSwapExtent(swapChainSupport.capabilities, setup->m_window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
    createInfo.surface = setup->m_surface;

    uint32_t present_index = setup->indices.value() & UINT32_MAX;
    uint32_t graphics_index = setup->indices.value() >> 32;
    uint32_t queueFamilyIndices[] = { graphics_index, present_index };

    if (graphics_index != present_index) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = colorSurfaceFormat.format;
    createInfo.imageColorSpace = colorSurfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;
    result = m_logicalDevice.createSwapchainKHR(&createInfo,
        setup->m_alloca_callback, &m_swapChain, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to create color swap chain!");
    }

    result = m_logicalDevice.getSwapchainImagesKHR(
        m_swapChain, &imageCount, nullptr, setup->m_dispatch_loader);
    if (!result) {
        THROW("Failed to get swap chain image count");
    }
    color_chain.m_images = imageCount;
    result = m_logicalDevice.getSwapchainImagesKHR(m_swapChain, &imageCount,
        color_chain.m_images.data, setup->m_dispatch_loader);
    if (!result) {
        THROW("Failed to get swap chain images");
    }
    color_chain.m_format = colorSurfaceFormat.format;
    m_extent = extent;

    vk::SurfaceFormatKHR depthSurfaceFormat = { vk::Format::eD32Sfloat,
        vk::ColorSpaceKHR::eSrgbNonlinear };
    for (auto format : swapChainSupport.formats) {
        if (format.format == vk::Format::eD32Sfloat ||
            format.format == vk::Format::eD32SfloatS8Uint ||
            format.format == vk::Format::eD16UnormS8Uint ||
            format.format == vk::Format::eD24UnormS8Uint ||
            format.format == vk::Format::eD16UnormS8Uint) {
            depthSurfaceFormat = format;
            break;
        }
    }

    depth_chain.m_format = findDepthFormat(setup->m_gpuDevice);
}

void jvr::renderer::createImageViews()
{
    config->attachments[0].format = color_chain.m_format;
    color_chain.m_images_views = color_chain.m_images.size();

    for (size_t i = 0; i < color_chain.m_images.size(); i++) {
        vk::ImageViewCreateInfo createInfo{};
        createInfo.sType = vk::StructureType::eImageViewCreateInfo;
        createInfo.image = color_chain.m_images[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = color_chain.m_format;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask =
            vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        result = m_logicalDevice.createImageView(&createInfo,
            setup->m_alloca_callback, &color_chain.m_images_views[i],
            setup->m_dispatch_loader);
        if (!result) {
            THROW("failed to create color image views!");
        }
    }
}

void jvr::renderer::createDepthViews()
{
    vk::Format format = depth_chain.m_format;
    uint32_t count = color_chain.m_images.size();

    config->attachments[1].format = depth_chain.m_format;
    m_depth_memories = count;
    depth_chain.m_images = count;
    depth_chain.m_images_views = count;

    bool hasStencil = format == vk::Format::eD32SfloatS8Uint ||
                      format == vk::Format::eD24UnormS8Uint ||
                      format == vk::Format::eD16UnormS8Uint;
    depth_chain.m_images_views = color_chain.m_images.size();

    for (uint32_t i = 0; i < count; i++) {
        vk::ImageCreateInfo img{};
        img.sType = vk::StructureType::eImageCreateInfo;
        img.imageType = vk::ImageType::e2D;
        img.format = format;
        img.extent = vk::Extent3D{ m_extent.width, m_extent.height, 1 };
        img.mipLevels = 1;
        img.arrayLayers = 1;
        img.samples = vk::SampleCountFlagBits::e1;
        img.tiling = vk::ImageTiling::eOptimal;
        img.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        img.sharingMode = vk::SharingMode::eExclusive;
        img.initialLayout = vk::ImageLayout::eUndefined;

        result = m_logicalDevice.createImage(&img, setup->m_alloca_callback,
            &depth_chain.m_images[i], setup->m_dispatch_loader);
        if (!result) {
            THROW("failed to create depth image!");
        }

        vk::MemoryRequirements reqs;
        m_logicalDevice.getImageMemoryRequirements(
            depth_chain.m_images[i], &reqs, setup->m_dispatch_loader);

        vk::MemoryAllocateInfo alloc{};
        alloc.sType = vk::StructureType::eMemoryAllocateInfo;
        alloc.allocationSize = reqs.size;
        alloc.memoryTypeIndex = setup->findMemoryType(
            reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        result = m_logicalDevice.allocateMemory(&alloc,
            setup->m_alloca_callback, &m_depth_memories[i],
            setup->m_dispatch_loader);

        if (!result)
            THROW("failed to allocate depth memory!");

        m_logicalDevice.bindImageMemory(depth_chain.m_images[i],
            m_depth_memories[i], 0, setup->m_dispatch_loader);

        vk::ImageViewCreateInfo view{};
        view.sType = vk::StructureType::eImageViewCreateInfo;
        view.image = depth_chain.m_images[i];
        view.viewType = vk::ImageViewType::e2D;
        view.format = format;
        view.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (hasStencil)
            view.subresourceRange.aspectMask |=
                vk::ImageAspectFlagBits::eStencil;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.layerCount = 1;

        result = m_logicalDevice.createImageView(&view,
            setup->m_alloca_callback, &depth_chain.m_images_views[i],
            setup->m_dispatch_loader);
        if (!result)
            THROW("failed to create depth image view!");
    }
}
