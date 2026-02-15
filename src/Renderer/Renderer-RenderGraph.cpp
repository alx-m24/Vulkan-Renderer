#include "pch.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/Buffer/BufferDescription.hpp"
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
        pass->renderer = this;

        for (const PipelineDescription& pipelineDesc : pass->getPipelineDescriptions()) {
            m_pipelines.emplace_back(std::make_shared<Pipeline>(pipelineDesc.name));

            CreateVulkanPipeline(pipelineDesc, m_pipelines.back()->pipeline, m_pipelines.back()->pipelineLayout);

		    pass->pipelines.insert({ pipelineDesc.name, m_pipelines.back()});
        }
        for (const BufferDescription& bufferDesc : pass->getBufferDescriptions()) {
            std::shared_ptr<Buffer> buffer;

            switch (bufferDesc.usage) {
                case BufferUsage::VERTEX_BUFFER: buffer = CreateBuffer<VertexBuffer>(bufferDesc); break;
                case BufferUsage::UNIFORM_BUFFER: buffer = CreateBuffer<UniformBuffer>(bufferDesc); break;
                case BufferUsage::TRANSFER_BUFFER: buffer = CreateBuffer<TransferBuffer>(bufferDesc); break;
            }
            
		    pass->buffers.insert({ bufferDesc.name, buffer });
        }
    }
}
