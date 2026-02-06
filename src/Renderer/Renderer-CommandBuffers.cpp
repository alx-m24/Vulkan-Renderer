#include "pch.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderGraph/RenderGraph.hpp"

void Renderer::CreateCommandPool() {
    vk::CommandPoolCreateInfo poolInfo {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = m_graphicsFamilyIndex
    };
    m_commandPool = vk::raii::CommandPool(m_device, poolInfo);
}

void Renderer::CreateCommandBuffers() {
    vk::CommandBufferAllocateInfo bufferInfo {
        .commandPool = m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT // 1 buffer per frame
    };
    
    m_commandBuffers.clear();
    m_commandBuffers = vk::raii::CommandBuffers(m_device, bufferInfo);
}

void Renderer::CreateSyncObjects() {
    assert(m_presentCompleteSemaphores.empty() && m_renderFinishedSemaphores.empty() && m_framesInFlightFence.empty());
	
    for (size_t i = 0; i < m_swapChainImages.size(); i++)
	{
		m_renderFinishedSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo());
	}

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
	    m_presentCompleteSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo());
	    m_framesInFlightFence.emplace_back(m_device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
	}
}
