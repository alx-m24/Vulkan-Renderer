#pragma once 

#include <vk_mem_alloc.h>

#include "BufferDescription.hpp"

class Buffer {
    friend class Renderer;

    protected:
        std::string name = "";
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = {};
        vk::BufferUsageFlags usage = {};

    public:
        size_t size = 0;

    public:
        Buffer(const std::string& name) : name(name) {}

        Buffer(const std::string& name, size_t size) : name(name), size(size) {}

        Buffer(const std::string& name, BufferUsage bufferUsage) : name(name), usage(getUsageFlags(bufferUsage)) {}
        Buffer(const std::string& name, size_t size, BufferUsage bufferUsage) : name(name), usage(getUsageFlags(bufferUsage)), size(size) {}

    private:
        static vk::BufferUsageFlags getUsageFlags(BufferUsage usage);

    public:
        std::string getName() const { return name; }

        bool operator==(const Buffer& other) const {
            return name == other.name;
        }
};

template<typename T>
concept Buffer_T = std::is_base_of_v<Buffer, T>;

class VertexBuffer : public Buffer {
    public:
        VertexBuffer(const std::string& name) : Buffer(name, BufferUsage::VERTEX_BUFFER) {}
        VertexBuffer(const std::string& name, size_t size) : Buffer(name, size, BufferUsage::VERTEX_BUFFER) {}
};

class UniformBuffer : public Buffer {
    public:
        UniformBuffer(const std::string& name) : Buffer(name, BufferUsage::UNIFORM_BUFFER) {}
        UniformBuffer(const std::string& name, size_t size) : Buffer(name, size, BufferUsage::UNIFORM_BUFFER) {}
};

class TransferBuffer : public Buffer {
    public:
        TransferBuffer(const std::string& name) : Buffer(name, BufferUsage::TRANSFER_BUFFER) {}
        TransferBuffer(const std::string& name, size_t size) : Buffer(name, size, BufferUsage::TRANSFER_BUFFER) {}
};
