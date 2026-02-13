#include "pch.hpp" 
#include "Renderer/Renderer.hpp"

#include "Utils.hpp"
#include "Renderer/Shader/Shader.hpp"
#include "Renderer/Vertex/VertexInfo.hpp"
#include "Renderer/Renderer-Exceptions.hpp"
#include "Renderer/Rasterizer/Rasterizer.hpp"
#include "Renderer/Pipeline/PipelineDescription.hpp"

static vk::raii::ShaderModule CreateShaderModule(const ByteArray& code, const vk::raii::Device& device) {
    assert(code.size() % 4 == 0 && "SPIR-V size must be multiple of 4");

    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size(),
        .pCode    = reinterpret_cast<const uint32_t*>(code.data())
    };

	vk::raii::ShaderModule shaderModule{device, createInfo};

	return shaderModule;
}

static vk::VertexInputRate toVKVertexInputRate(const VertexInputRate& rate) {
    switch (rate) {
        case VertexInputRate::PER_VERTEX: return vk::VertexInputRate::eVertex;
        case VertexInputRate::PER_INSTANCE: return vk::VertexInputRate::eInstance;
    };

    return {};
}

static vk::Format toVKFormat(const VertexAttributeType& type) {
    switch (type) {
        case VertexAttributeType::FLOAT: return vk::Format::eR32Sfloat;
        case VertexAttributeType::VEC_2: return vk::Format::eR32G32Sfloat;
        case VertexAttributeType::VEC_3: return vk::Format::eR32G32B32Sfloat;
        case VertexAttributeType::VEC_4: return vk::Format::eR32G32B32A32Sfloat;
        case VertexAttributeType::INT: return vk::Format::eR32Sint;
        case VertexAttributeType::IVEC_2: return vk::Format::eR32G32Sint;
        case VertexAttributeType::IVEC_3: return vk::Format::eR32G32B32Sint;
        case VertexAttributeType::IVEC_4: return vk::Format::eR32G32B32A32Sint;
        case VertexAttributeType::UINT: return vk::Format::eR32Uint;
        case VertexAttributeType::UVEC_2: return vk::Format::eR32G32Uint;
        case VertexAttributeType::UVEC_3: return vk::Format::eR32G32B32Uint;
        case VertexAttributeType::UVEC_4: return vk::Format::eR32G32B32A32Uint;
        case VertexAttributeType::BYTE_4_NORM: return vk::Format::eR8G8B8A8Unorm;
        case VertexAttributeType::SHORT_2_NORM: return vk::Format::eR16G16Unorm;
    };

    return {};
}

static vk::PipelineVertexInputStateCreateInfo getVKVertexInputInfo(const VertexInfo& vertexInfo) {
    std::vector<vk::VertexInputBindingDescription> bindings;
    std::vector<vk::VertexInputAttributeDescription> attributes;

    bindings.reserve(vertexInfo.bindings.size());
    attributes.reserve(vertexInfo.attributes.size());

    for (const VertexBindingDescription& binding : vertexInfo.bindings) {
        bindings.emplace_back(
                vk::VertexInputBindingDescription {
                    .binding = binding.binding,
                    .stride = binding.stride,
                    .inputRate = toVKVertexInputRate(binding.inputRate)
                });
        
    }

    for (const VertexAttributeDescription& attribute : vertexInfo.attributes) {
        attributes.emplace_back(
                vk::VertexInputAttributeDescription {
                    .location = attribute.location,
                    .binding = attribute.binding,
                    .format = toVKFormat(attribute.type),
                    .offset = attribute.offset,
                });
    }

    vk::PipelineVertexInputStateCreateInfo inputInfo {
        .vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size()),
        .pVertexBindingDescriptions = bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size()),
        .pVertexAttributeDescriptions = attributes.data()
    };

    return inputInfo;
}

static vk::Bool32 toVKBool(bool val) {
    return val ? vk::True : vk::False;
}

static vk::PolygonMode toVKPolygonMode(PolygonMode mode) {
    switch (mode) {
        case PolygonMode::FILL: return vk::PolygonMode::eFill;
        case PolygonMode::POINT: return vk::PolygonMode::ePoint;
        case PolygonMode::LINE: return vk::PolygonMode::eLine;
    }
    return {};
}

static vk::CullModeFlags toVKCullMode(CullFace cullmode) {
    switch (cullmode) {
        case CullFace::NONE: return vk::CullModeFlagBits::eNone;
        case CullFace::FRONT: return vk::CullModeFlagBits::eFront;
        case CullFace::BACK: return vk::CullModeFlagBits::eBack;
    }

    return {};
}

static vk::FrontFace toVKFrontFace(FrontFace frontFace) {
    switch (frontFace) {
        case FrontFace::CW: return vk::FrontFace::eClockwise;
        case FrontFace::CCW: return vk::FrontFace::eCounterClockwise;
    }

    return {};
}

static vk::PipelineRasterizationStateCreateInfo getVKRasterizer(Rasterizer rasterizer) {
    vk::PipelineRasterizationStateCreateInfo createInfo {
        .depthClampEnable = toVKBool(rasterizer.depthClampEnable),
        .rasterizerDiscardEnable = toVKBool(rasterizer.discardEnable),
        .polygonMode = toVKPolygonMode(rasterizer.polygonMode),
        .cullMode = toVKCullMode(rasterizer.faceCulling.cullFace), 
        .frontFace = toVKFrontFace(rasterizer.faceCulling.frontFace), 
        .depthBiasEnable = vk::False, 
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    return createInfo;
}

std::vector<vk::PipelineShaderStageCreateInfo> Renderer::getShaderStages(GraphicsShader& shader) const {
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

    {
        VertexStage& vertex = shader.vertexStage;
        
        assert(vertex.module && "Vertex Stage should have a module");

        if (vertex.module->module == VK_NULL_HANDLE) {
            std::expected<ByteArray, ReadFileError> rawDataExpected = readRawFile(vertex.module->path);

            if (!rawDataExpected) {
                throw PipelineCreation_Error("Error in reading " + vertex.module->path.string() + ": " + to_string(rawDataExpected.error()));
            }

            vertex.module->module = CreateShaderModule(*rawDataExpected, m_device);
        }

        shaderStages.emplace_back(
                vk::PipelineShaderStageCreateInfo {
                    .stage = vk::ShaderStageFlagBits::eVertex,
                    .module = vertex.module->module,
                    .pName = vertex.entry.c_str()
                }
            );
    }
    {
        FragmentStage& fragment = shader.fragmentStage;
        
        assert(fragment.module && "Fragment Stage should have a module");

        if (fragment.module->module == VK_NULL_HANDLE) {
            std::expected<ByteArray, ReadFileError> rawDataExpected = readRawFile(fragment.module->path);

            if (!rawDataExpected) {
                throw PipelineCreation_Error("Error in reading " + fragment.module->path.string() + ": " + to_string(rawDataExpected.error()));
            }

            fragment.module->module = CreateShaderModule(*rawDataExpected, m_device);
        }
        
        shaderStages.emplace_back(
                vk::PipelineShaderStageCreateInfo {
                    .stage = vk::ShaderStageFlagBits::eFragment,
                    .module = fragment.module->module,
                    .pName = fragment.entry.c_str()
                }
            );
    }
    if (shader.geometryStage) {
        GeometryStage& geometry = shader.geometryStage.value();
        
        assert(geometry.module && "Geometry Stage should have a module");

        if (geometry.module->module == VK_NULL_HANDLE) {
            std::expected<ByteArray, ReadFileError> rawDataExpected = readRawFile(geometry.module->path);

            if (!rawDataExpected) {
                throw PipelineCreation_Error("Error in reading " + geometry.module->path.string() + ": " + to_string(rawDataExpected.error()));
            }


            geometry.module->module = CreateShaderModule(*rawDataExpected, m_device);
        }
        
        shaderStages.emplace_back(
                vk::PipelineShaderStageCreateInfo {
                    .stage = vk::ShaderStageFlagBits::eGeometry,
                    .module = geometry.module->module,
                    .pName = geometry.entry.c_str()
                }
            );
    }

    return shaderStages;
}

void Renderer::CreateVulkanPipeline(PipelineDescription desc, vk::raii::Pipeline& pipeline, vk::raii::PipelineLayout& layout) const {
    
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = getShaderStages(desc.shader);

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = getVKVertexInputInfo(desc.vertexInfo);

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly { .topology = vk::PrimitiveTopology::eTriangleList };
    vk::PipelineViewportStateCreateInfo viewPortState { .viewportCount = 1, .scissorCount = 1 };

    vk::PipelineRasterizationStateCreateInfo rasterizer = getVKRasterizer(desc.rasterizer);

    vk::PipelineMultisampleStateCreateInfo multiSampling {};
    switch (desc.sampleCount) {
        case SampleCount::ONE:
            multiSampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
            multiSampling.sampleShadingEnable = false;
            break;
        case SampleCount::TWO:
            multiSampling.rasterizationSamples = vk::SampleCountFlagBits::e2;
            multiSampling.sampleShadingEnable = true;
            break;
        case SampleCount::FOUR:
            multiSampling.rasterizationSamples = vk::SampleCountFlagBits::e4;
            multiSampling.sampleShadingEnable = true;
            break;
        case SampleCount::EIGHT:
            multiSampling.rasterizationSamples = vk::SampleCountFlagBits::e8;
            multiSampling.sampleShadingEnable = true;
            break;
        case SampleCount::SIXTEEN:
            multiSampling.rasterizationSamples = vk::SampleCountFlagBits::e16;
            multiSampling.sampleShadingEnable = true;
            break;
        case SampleCount::THIRY_TWO:
            multiSampling.rasterizationSamples = vk::SampleCountFlagBits::e32;
            multiSampling.sampleShadingEnable = true;
            break;
        case SampleCount::SIXTY_FOUR:
            multiSampling.rasterizationSamples = vk::SampleCountFlagBits::e64;
            multiSampling.sampleShadingEnable = true;
            break;
    }

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
        .colorWriteMask = vk::ColorComponentFlagBits::eR |
                        vk::ColorComponentFlagBits::eG |
                        vk::ColorComponentFlagBits::eB |
                        vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment};

    switch (desc.colorBlend) {
        case BlendMode::NONE:
            colorBlendAttachment.blendEnable = vk::False;
            break;
        case BlendMode::ONE_MINUS_SRC_ALPHA:
            colorBlendAttachment.blendEnable = vk::True;

             colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
             colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
             colorBlendAttachment.colorBlendOp        = vk::BlendOp::eAdd;

             colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
             colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
             colorBlendAttachment.alphaBlendOp        = vk::BlendOp::eAdd;
            break;
    }

    vk::PipelineDepthStencilStateCreateInfo depthStencil {
        .depthTestEnable       = toVKBool(desc.depthTestEnabled),                      // enable depth testing
        .depthWriteEnable      = toVKBool(desc.depthWriteEnabled),                      // enable writing to the depth buffer
        .depthCompareOp        = vk::CompareOp::eLess,          // default: fragments closer to camera pass
        .depthBoundsTestEnable = vk::False,                     // optional depth bounds test
        .stencilTestEnable     = vk::False,                     // enable if you need stencil
        .front                 = {},                            // stencil operations for front faces
        .back                  = {},                            // stencil operations for back faces
        .minDepthBounds        = 0.0f,                          // used if depthBoundsTestEnable = true
        .maxDepthBounds        = 1.0f
    };

    std::vector<vk::DynamicState> dynamicStates = {
		        vk::DynamicState::eViewport,
		        vk::DynamicState::eScissor};

	vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), 
        .pDynamicStates = dynamicStates.data()
    };

    std::vector<vk::Format> colorAttachmentFormats;
    colorAttachmentFormats.reserve(desc.colorAttachments.size());

    for (const ColorAttachmentFormat& format : desc.colorAttachments) {
        if (format == ColorAttachmentFormat::SWAPCHAIN_FORMAT) colorAttachmentFormats.emplace_back(m_SwapChainSurfaceFormat.format);
    }

    vk::PipelineLayoutCreateInfo layoutInfo {
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    };

    layout = vk::raii::PipelineLayout(m_device, layoutInfo);

	vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
	    {.stageCount          = static_cast<uint32_t>(shaderStages.size()),
	     .pStages             = shaderStages.data(),
	     .pVertexInputState   = &vertexInputInfo,
	     .pInputAssemblyState = &inputAssembly,
	     .pViewportState      = &viewPortState,
	     .pRasterizationState = &rasterizer,
	     .pMultisampleState   = &multiSampling,
         .pDepthStencilState  = &depthStencil,
	     .pColorBlendState    = &colorBlending,
	     .pDynamicState       = &dynamicState,
	     .layout              = layout,
	     .renderPass          = nullptr},
	    {.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size()), .pColorAttachmentFormats = colorAttachmentFormats.data()}};

    pipeline = vk::raii::Pipeline(m_device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}
