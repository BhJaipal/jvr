// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "array.h"
#include "render.h"

void jvr_render_drawFrame(jvr_renderer *this, uint32_t imageIndex)
{
    vkCmdEndRenderPass(this->m_commandBuffer);

    if (vkEndCommandBuffer(this->m_commandBuffer) != VK_SUCCESS) {
        THROW("failed to record command buffer!", this->result);
    }

    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { this->imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &this->m_commandBuffer;

    VkSemaphore signalSemaphores[] = { this->renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    this->result = vkQueueSubmit(
        this->setup->graphicsQueue, 1, &submitInfo, this->inFlightFence);
    if (this->result != VK_SUCCESS) {
        THROW("failed to submit draw command buffer!", this->result);
    }

    VkPresentInfoKHR presentInfo = { 0 };
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { this->m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(this->setup->presentQueue, &presentInfo);
}
void jvr_render_recordCommandBuffer(jvr_renderer *this, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL
    };

    this->result = vkBeginCommandBuffer(this->m_commandBuffer, &beginInfo);
    if (this->result != VK_SUCCESS) {
        THROW("failed to begin recording command buffer!", this->result);
    }

    VkRenderPassBeginInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->m_render_pass;
    renderPassInfo.framebuffer =
        JVR_ELEM(this->swapChainFramebuffers, VkFramebuffer, imageIndex);
    renderPassInfo.renderArea.offset = (VkOffset2D){ 0, 0 };
    renderPassInfo.renderArea.extent = this->m_extent;

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = this->config->clear_buff;

    vkCmdBeginRenderPass(
        this->m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(this->m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        this->m_pipeline);

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)this->m_extent.width;
    viewport.height = (float)this->m_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(this->m_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = { 0 };
    scissor.offset = (VkOffset2D){ 0, 0 };
    scissor.extent = this->m_extent;
    vkCmdSetScissor(this->m_commandBuffer, 0, 1, &scissor);
}
