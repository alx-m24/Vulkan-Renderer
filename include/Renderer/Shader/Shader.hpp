#pragma once

#include <filesystem>
#include <optional>
#include <utility>

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

inline GraphicsShader make_graphicsShader(const std::filesystem::path& modulePath, std::string vertEntry = "vertMain", std::string fragEntry = "fragMain") {
    std::shared_ptr<ShaderModule> module = std::make_shared<ShaderModule>(modulePath);

    return GraphicsShader(VertexStage{ .module = module, .entry =vertEntry }, FragmentStage{ .module = module, .entry = fragEntry } );
}

inline GraphicsShader make_graphicsShader(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath,
        std::string vertEntry = "vertMain", std::string fragEntry = "fragMain") {

    if (vertPath == fragPath) {
        return make_graphicsShader(vertPath, vertEntry, fragEntry);
    }

    std::shared_ptr<ShaderModule> vertModule = std::make_shared<ShaderModule>(vertPath);
    std::shared_ptr<ShaderModule> fragModule = std::make_shared<ShaderModule>(fragPath);

    return GraphicsShader(VertexStage{ .module = vertModule, .entry =vertEntry }, FragmentStage{ .module = fragModule, .entry = fragEntry } );
}

template<ShaderUsage U, typename... Args>
inline Shader<U> make_shader(Args&&... args) {
    if constexpr (U == ShaderUsage::GRAPHICS) return make_graphicsShader(std::forward<Args>(args)...);
    if constexpr (U == ShaderUsage::COMPUTE) return {}; // TODO

    return {};
}

