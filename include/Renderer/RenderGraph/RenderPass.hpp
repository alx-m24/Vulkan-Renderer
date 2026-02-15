#pragma once

#include "ImageResource.hpp"
#include "Renderer/Buffer/BufferDescription.hpp"
#include "Renderer/Pipeline/PipelineDescription.hpp"

#include <unordered_map>

// Forward declaration
class Renderer;
class Pipeline;
class Buffer;

namespace RenderGraph {
    class RenderPass {
        public:
            friend class RenderGraph;
            friend class ::Renderer;
        protected:
            std::string name = "";

            std::vector<std::string> reads {};
            std::vector<std::string> writes {};

            // Execution-time only
            std::unordered_map<std::string, ImageResource*> readImages;
            std::unordered_map<std::string, ImageResource*> writeImages;

            std::unordered_map<std::string, std::shared_ptr<const Pipeline>> pipelines;
            std::unordered_map<std::string, std::shared_ptr<const Buffer>> buffers;

            Renderer* renderer;

        public:
            RenderPass(std::string&& name, const std::vector<std::string>& reads, const std::vector<std::string>& writes)
                : name(std::move(name)),
                  reads(std::move(reads)),
                  writes(std::move(writes)) {}
            virtual ~RenderPass() = default;

        public:
            virtual std::span<const PipelineDescription> getPipelineDescriptions() const = 0;
            virtual std::span<const BufferDescription> getBufferDescriptions() const = 0;

        public:
            virtual void BeginPass(const vk::raii::CommandBuffer& cmd) = 0;
            virtual void RunPass(const vk::raii::CommandBuffer& cmd) = 0;
            virtual void EndPass(const vk::raii::CommandBuffer& cmd) = 0;

        public:
            bool operator==(const RenderPass& pass) const {
                return name == pass.name;
            }
    };

    template<typename T>
    concept isRenderPass = std::is_base_of_v<RenderPass, T>;
}
