#pragma once

#include <filesystem>
#include <optional>

namespace vk {
    namespace raii {
        class ShaderModule;
    }
}

class ShaderModule {
    friend class Renderer;

    private:
        vk::raii::ShaderModule module = VK_NULL_HANDLE;
        std::filesystem::path path {};

    public:
        ShaderModule() = default;
        ShaderModule(const std::filesystem::path& path) : path(path) {}
};

enum class ShaderStageType {
    VERTEX = 0,
    FRAGMENT,
    GEOMETRY 
};

template<ShaderStageType shaderStage>
struct ShaderStage {
    std::shared_ptr<ShaderModule> module {};
    std::string entry = "";

    constexpr static ShaderStageType getStageType() { return shaderStage; }
};

using VertexStage = ShaderStage<ShaderStageType::VERTEX>;
using FragmentStage = ShaderStage<ShaderStageType::FRAGMENT>;
using GeometryStage = ShaderStage<ShaderStageType::GEOMETRY>;

enum class ShaderUsage {
    GRAPHICS = 0,
    COMPUTE 
};

template<ShaderUsage usage>
class Shader {
    friend class Renderer;
};

using GraphicsShader = Shader<ShaderUsage::GRAPHICS>;
using ComputeShader = Shader<ShaderUsage::COMPUTE>;

template<>
class Shader<ShaderUsage::GRAPHICS> {
    friend class Renderer;
    private:
        VertexStage vertexStage;
        FragmentStage fragmentStage;
        std::optional<GeometryStage> geometryStage;

    public:
        Shader() = default;

        Shader(const VertexStage& vertex, const FragmentStage& fragment) 
            : vertexStage(vertex), fragmentStage(fragment) {}

        Shader(const VertexStage& vertex, const FragmentStage& fragment, const GeometryStage& geometry) 
            : vertexStage(vertex), fragmentStage(fragment), geometryStage(geometry) {}
};

template<>
class Shader<ShaderUsage::COMPUTE> {
    private:
        ShaderModule computeShader{}; 
};

