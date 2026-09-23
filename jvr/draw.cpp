// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "render.hpp"
#include <vulkan/vulkan_enums.hpp>

void jvr::renderer::drawFrame(uint32_t imageIndex)
{
    m_commandBuffer.endRenderPass(setup->m_dispatch_loader);

    m_commandBuffer.end(setup->m_dispatch_loader);

    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;

    vk::Semaphore waitSemaphores[] = { m_imageAvailableSemaphore };
    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    vk::Semaphore signalSemaphores[] = { m_renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    result = m_graphicsQueue.submit(
        1, &submitInfo, m_inFlightFence, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to submit draw command buffer!");
    }

    vk::PresentInfoKHR presentInfo{};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = m_presentQueue.presentKHR(&presentInfo, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to Present queue");
    }
}
uint32_t jvr::renderer::recordCommandBuffer()
{
    uint32_t imageIndex;
    result = m_logicalDevice.waitForFences(
        1, &m_inFlightFence, VK_TRUE, UINT64_MAX, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to wait for fences");
    }
    result = m_logicalDevice.resetFences(
        1, &m_inFlightFence, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to reset fences");
    }

    result = m_logicalDevice.acquireNextImageKHR(m_swapChain, UINT64_MAX,
        m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex,
        setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to acquireNextImageKHR next image");
    }

    m_commandBuffer.reset(
        vk::CommandBufferResetFlagBits(), setup->m_dispatch_loader);

    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;

    result = m_commandBuffer.begin(&beginInfo, setup->m_dispatch_loader);
    if (!result) {
        THROW("failed to begin recording command buffer!");
    }

    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    renderPassInfo.renderPass = m_render_pass;
    renderPassInfo.framebuffer = m_framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    renderPassInfo.renderArea.extent = m_extent;

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = config->clear_buff;

    m_commandBuffer.beginRenderPass(&renderPassInfo,
        vk::SubpassContents::eInline, setup->m_dispatch_loader);

    m_commandBuffer.bindPipeline(
        vk::PipelineBindPoint::eGraphics, m_pipeline, setup->m_dispatch_loader);

    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_extent.width;
    viewport.height = (float)m_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    m_commandBuffer.setViewport(0, 1, &viewport, setup->m_dispatch_loader);

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{ 0, 0 };
    scissor.extent = m_extent;
    m_commandBuffer.setScissor(0, 1, &scissor, setup->m_dispatch_loader);
    return imageIndex;
}
