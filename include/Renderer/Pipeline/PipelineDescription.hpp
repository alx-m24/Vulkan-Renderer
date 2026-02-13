#pragma once

#include <type_traits>

#include "Renderer/Shader/Shader.hpp"
#include "Renderer/Vertex/VertexInfo.hpp"
#include "Renderer/Rasterizer/Rasterizer.hpp"
#include "Renderer/Rasterizer/Rasterizer.hpp"

enum class SampleCount {
    ONE = 0,
    TWO,
    FOUR,
    EIGHT,
    SIXTEEN,
    THIRY_TWO,
    SIXTY_FOUR
};

enum class BlendMode {
    NONE = 0,
    ONE_MINUS_SRC_ALPHA
};

enum class ColorAttachmentFormat {
    NONE = 0,
    SWAPCHAIN_FORMAT,
    // TODO: Add formats
};

struct PipelineDescription {
    std::string name = "";
    GraphicsShader shader {};
    VertexInfo vertexInfo = {};
    Rasterizer rasterizer {};
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;
    SampleCount sampleCount = SampleCount::ONE;
    BlendMode colorBlend = BlendMode::NONE;
    std::vector<ColorAttachmentFormat> colorAttachments {};

    bool operator==(const PipelineDescription& other) const {
        return this->name == other.name;
    };
};

// WHAT IT ENFORCES:
//  1. Scissor and Viewport dynamic states
//  2. Topology as triangles
