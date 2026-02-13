#pragma once 

class Pipeline {
    friend class Renderer;
    private:
        std::string name = "";
        vk::raii::Pipeline pipeline = VK_NULL_HANDLE;
        vk::raii::PipelineLayout pipelineLayout = VK_NULL_HANDLE;

    public:
        Pipeline() = default;
        Pipeline(const std::string& name) : name(name) {}

    public:
        void Bind(const vk::CommandBuffer& cmdBuffer) const {
            cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
        }
};
