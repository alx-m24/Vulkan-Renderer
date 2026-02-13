#include "pch.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/Shader/Shader.hpp"
#include "Renderer/Pipeline/Pipeline.hpp"
#include "Renderer/Pipeline/PipelineDescription.hpp"
#include "Renderer/RenderGraph/RenderGraph.hpp"

class ForwardPass : public RenderGraph::RenderPass {
    public:
        ForwardPass() : RenderGraph::RenderPass("ForwardPass", {}, {"BackBuffer"}), shaderModule(std::make_shared<ShaderModule>("shaders/shader.spv")) {}

    public:
        const std::shared_ptr<ShaderModule> shaderModule;
        const VertexStage vert { .module = shaderModule, .entry = "vertMain" };
        const FragmentStage frag { .module = shaderModule, .entry = "fragMain" };

        const std::array<PipelineDescription, 1u> s_pipelines {
            PipelineDescription{ 
                .name = "Main",
                .shader = GraphicsShader(vert, frag),
                .vertexInfo = {},
                .rasterizer = Rasterizer { .faceCulling = { .cullFace = CullFace::BACK, .frontFace = FrontFace::CW } },
                .depthTestEnabled = false, .depthWriteEnabled = false,
                .colorAttachments = { ColorAttachmentFormat::SWAPCHAIN_FORMAT }
            }
        };

        virtual std::span<const PipelineDescription> getPipelineDescriptions() const override {
            return s_pipelines;
        }

    public:
        void BeginPass([[maybe_unused]] const vk::raii::CommandBuffer& cmd) override {
            ImageResource* backBuffer = writeImages["BackBuffer"];

            vk::ClearValue clearColor = vk::ClearColorValue(1.0f, 0.0f, 0.0f, 1.0f);
		    vk::RenderingAttachmentInfo attachmentInfo = {
		        .imageView   = backBuffer->view,
		        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		        .loadOp      = vk::AttachmentLoadOp::eClear,
		        .storeOp     = vk::AttachmentStoreOp::eStore,
		        .clearValue  = clearColor};
		    vk::RenderingInfo renderingInfo = {
		        .renderArea           = {.offset = {0, 0}, .extent = backBuffer->extent},
		        .layerCount           = 1,
		        .colorAttachmentCount = 1,
		        .pColorAttachments    = &attachmentInfo};
		    cmd.beginRendering(renderingInfo);
		    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), backBuffer->extent));
        }

        void RunPass([[maybe_unused]] const vk::raii::CommandBuffer& cmd) override {
            ImageResource* backBuffer = writeImages["BackBuffer"];

            pipelines.at("Main")->Bind(cmd);

		    cmd.setViewport(0, 
                    vk::Viewport(0.0f, 0.0f, static_cast<float>(backBuffer->extent.width), static_cast<float>(backBuffer->extent.height), 0.0f, 1.0f));
            cmd.draw(3, 1, 0, 0);
        }

        void EndPass([[maybe_unused]] const vk::raii::CommandBuffer& cmd) override {
            cmd.endRendering();
        }
};

int main() {
    Renderer renderer;
    renderer.Init("Renderer - Demo");

    {
        RenderGraph::RenderGraph renderGraph;

        renderGraph.AddRenderPass<ForwardPass>();

        renderer.SetRenderGraph(std::make_unique<RenderGraph::RenderGraph>(std::move(renderGraph)));
    }
    
    while (renderer.isRunning()) {
        renderer.Update();

        renderer.Render();
    }

    renderer.Shutdown();

    return EXIT_SUCCESS;
}
