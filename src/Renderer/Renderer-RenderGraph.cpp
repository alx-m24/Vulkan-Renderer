#include "pch.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/Pipeline/Pipeline.hpp"
#include "Renderer/Pipeline/PipelineDescription.hpp"

void Renderer::SetRenderGraph(std::unique_ptr<RenderGraph::RenderGraph> renderGraph) {
    m_renderGraph = std::move(renderGraph);

    m_renderGraph->AddResource(ImageResource {
                .name = "BackBuffer",
                .format = m_SwapChainSurfaceFormat.format,
                .extent = m_swapChainExtent,
                .usage = vk::ImageUsageFlagBits::eColorAttachment,
            });

    m_renderGraph->Compile();

    for (std::unique_ptr<RenderGraph::RenderPass>& pass : m_renderGraph->getOrderedNodesUnsafe()) {
        for (const PipelineDescription& pipelineDesc : pass->getPipelineDescriptions()) {
            m_pipelines.emplace_back(std::make_shared<Pipeline>(pipelineDesc.name));

            CreateVulkanPipeline(pipelineDesc, m_pipelines.back()->pipeline, m_pipelines.back()->pipelineLayout);

		    pass->pipelines.insert({ pipelineDesc.name, m_pipelines.back()});
        }
    }
}
