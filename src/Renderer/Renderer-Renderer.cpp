#include "pch.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderGraph/RenderPass.hpp"
#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/RenderGraph/ImageResource.hpp"

static void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer, vk::Image image, [[maybe_unused]] vk::Format format,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::ImageAspectFlags aspect) {
    vk::ImageMemoryBarrier barrier;

    barrier.setOldLayout(oldLayout)
            .setNewLayout(newLayout)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(image)
            .setSubresourceRange({aspect, 0, 1, 0, 1});

    vk::PipelineStageFlags sourceStage;      // When previous operations must finish
    vk::PipelineStageFlags destinationStage; // When subsequent operations can start

    // Configure synchronization for undefined-to-transfer layout transitions
    // This pattern is common when preparing images for data uploads
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        // Configure memory access permissions for upload preparation
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone)                // No previous access to synchronize
               .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);      // Enable transfer write operations

        // Set pipeline stage synchronization points for upload workflow
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;               // No previous work to wait for
        destinationStage = vk::PipelineStageFlagBits::eTransfer;           // Transfer operations can proceed

    // Configure synchronization for transfer-to-shader layout transitions
    // This pattern prepares uploaded images for shader sampling
    } 
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        // Configure memory access transition from writing to reading
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)       // Previous transfer writes must complete
               .setDstAccessMask(vk::AccessFlagBits::eShaderRead);         // Enable shader read access

        // Set pipeline stage synchronization for shader usage workflow
        sourceStage = vk::PipelineStageFlagBits::eTransfer;                // Transfer operations must complete
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;     // Fragment shaders can access
    }
    else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eDepthAttachmentOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
               .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        sourceStage = vk::PipelineStageFlagBits::eLateFragmentTests;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.setSrcAccessMask({})
               .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    }
    else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthAttachmentOptimal) {
        barrier.setSrcAccessMask({})
               .setDstAccessMask(
                   vk::AccessFlagBits::eDepthStencilAttachmentRead |
                   vk::AccessFlagBits::eDepthStencilAttachmentWrite
               );

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    }
    else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
               .setDstAccessMask({}); // Present doesn't need access
                                      //

        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
    }
    else {
        assert(false && "Unsupported image layout transition");
    }

    commandBuffer.pipelineBarrier(
        sourceStage,
        destinationStage,
        {},
        nullptr,
        nullptr, 
        barrier);
}

static void transitionIfNeeded(vk::raii::CommandBuffer& cmd, ImageResource& res, bool isWrite) {
    vk::ImageLayout newLayout = pickLayout(res, isWrite);

    if (res.initialLayout == newLayout)
        return;

    TransitionImageLayout(
        cmd,
        res.image,
        res.format,
        res.initialLayout,
        newLayout,
        (res.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment)
            ? vk::ImageAspectFlagBits::eDepth
            : vk::ImageAspectFlagBits::eColor
    );

    res.initialLayout = newLayout;
}

void Renderer::Render() {
    if (!m_renderGraph) {
        throw std::runtime_error("Render Graph not set: call Renderer::SetRenderGraph()");
    }

    auto fenceResult = m_device.waitForFences(*m_framesInFlightFence[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence");
    }

    auto [result, imageIndex] = m_swapChain.acquireNextImage(UINT64_MAX, *m_presentCompleteSemaphores[frameIndex], nullptr);
	if (result == vk::Result::eErrorOutOfDateKHR) {
		reCreateSwapChain();
		return;
	}
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
		throw std::runtime_error("failed to acquire swap chain image!");
	}
    m_renderGraph->BindExternalResource("BackBuffer", m_swapChainImages[imageIndex], m_swapChainImageViews[imageIndex]);

    const RenderGraph::RenderGraph::OrderedNodes& nodes = m_renderGraph->getOrderedNodes()->get();

    vk::raii::CommandBuffer& buffer = m_commandBuffers[frameIndex];

    m_device.resetFences(*m_framesInFlightFence[frameIndex]);
    buffer.reset();
    buffer.begin({});
        
    for (const std::unique_ptr<RenderGraph::RenderPass>& node : nodes) {
        for (auto& [name, img] : node->readImages) {
            transitionIfNeeded(buffer, *img, false);
        }
        
        for (auto& [name, img] : node->writeImages) {
            transitionIfNeeded(buffer, *img, true);
        }

        node->BeginPass(buffer);
        node->RunPass(buffer);
        node->EndPass(buffer);

    }
    
    // Submits to present command buffer
    TransitionImageLayout(buffer, m_swapChainImages[imageIndex], m_SwapChainSurfaceFormat.format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, vk::ImageAspectFlagBits::eColor);

    buffer.end();
    
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount   = 1,
	    .pWaitSemaphores      = &*m_presentCompleteSemaphores[frameIndex], 
        .pWaitDstStageMask    = &waitDestinationStageMask, 
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*m_commandBuffers[frameIndex], 
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*m_renderFinishedSemaphores[imageIndex]
    };
    m_graphicsQueue.submit(submitInfo, *m_framesInFlightFence[frameIndex]);
    
    vk::PresentInfoKHR presentInfoKHR {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*m_renderFinishedSemaphores[imageIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &*m_swapChain,
        .pImageIndices      = &imageIndex
    };
    result = m_presentQueue.presentKHR(presentInfoKHR);

    if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || m_frameBufferResized) {
        m_frameBufferResized = false;
    	reCreateSwapChain();
    }
    else {
    	assert(result == vk::Result::eSuccess);
    }

    frameIndex = (frameIndex + 1u) % MAX_FRAMES_IN_FLIGHT;
}
