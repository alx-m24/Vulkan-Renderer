#include "pch.hpp"
#include "stb/stb_image.h"

#include "Renderer/Renderer.hpp"

#include "Renderer/RenderGraph/RenderGraph.hpp"

static std::vector<char> readFile(const std::string &filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}
	std::vector<char> buffer(file.tellg());
	file.seekg(0, std::ios::beg);
	file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
	file.close();
	return buffer;
}

[[nodiscard]] vk::raii::ShaderModule createShaderModule(vk::raii::Device& device, const std::vector<char> &code) 
{
	vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t *>(code.data())};
	vk::raii::ShaderModule     shaderModule{device, createInfo};

	return shaderModule;
}


class ForwardPass : public RenderGraph::RenderPass {
    private: 
        vk::raii::PipelineLayout pipelineLayout = VK_NULL_HANDLE;
        vk::raii::Pipeline graphicsPipeline = VK_NULL_HANDLE;

    public:
        ForwardPass(vk::raii::Device& device, vk::SurfaceFormatKHR swapChainSurfaceFormat) : RenderGraph::RenderPass("ForwardPass", {}, {"BackBuffer"}) {
            vk::raii::ShaderModule shaderModule = createShaderModule(device, readFile("shaders/shader.spv"));

		    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{.stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"};
		    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{.stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
		    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		    vk::PipelineVertexInputStateCreateInfo   vertexInputInfo;
		    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
		    vk::PipelineViewportStateCreateInfo      viewportState{.viewportCount = 1, .scissorCount = 1};

		    vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable = vk::False, .rasterizerDiscardEnable = vk::False, .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack, .frontFace = vk::FrontFace::eClockwise, .depthBiasEnable = vk::False, .depthBiasSlopeFactor = 1.0f, .lineWidth = 1.0f};

		    vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};

		    vk::PipelineColorBlendAttachmentState colorBlendAttachment{.blendEnable    = vk::False,
		                                                               .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

		    vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment};

		    std::vector dynamicStates = {
		        vk::DynamicState::eViewport,
		        vk::DynamicState::eScissor};
		    vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()};

		    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 0, .pushConstantRangeCount = 0};

		    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

		    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
		        {.stageCount          = 2,
		         .pStages             = shaderStages,
		         .pVertexInputState   = &vertexInputInfo,
		         .pInputAssemblyState = &inputAssembly,
		         .pViewportState      = &viewportState,
		         .pRasterizationState = &rasterizer,
		         .pMultisampleState   = &multisampling,
		         .pColorBlendState    = &colorBlending,
		         .pDynamicState       = &dynamicState,
		         .layout              = pipelineLayout,
		         .renderPass          = nullptr},
		        {.colorAttachmentCount = 1, .pColorAttachmentFormats = &swapChainSurfaceFormat.format}};

		    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
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

		    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
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

        renderGraph.AddRenderPass<ForwardPass>(renderer.getDevice(), renderer.getSurfaceFormat());

        renderer.SetRenderGraph(std::make_unique<RenderGraph::RenderGraph>(std::move(renderGraph)));
    }

    while (renderer.isRunning()) {
        renderer.Update();

        renderer.Render();
    }

    renderer.Shutdown();

    return EXIT_SUCCESS;
}
