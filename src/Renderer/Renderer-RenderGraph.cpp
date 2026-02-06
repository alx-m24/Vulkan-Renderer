#include "pch.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderGraph/RenderGraph.hpp"

void Renderer::SetRenderGraph(std::unique_ptr<RenderGraph::RenderGraph> renderGraph) {
    m_renderGraph = std::move(renderGraph);

    m_renderGraph->AddResource(ImageResource {
                .name = "BackBuffer",
                .format = m_SwapChainSurfaceFormat.format,
                .extent = m_swapChainExtent,
                .usage = vk::ImageUsageFlagBits::eColorAttachment,
            });

    m_renderGraph->Compile();
}
