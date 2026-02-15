#include "pch.hpp" 
#include "Renderer/Buffer/Buffer.hpp"

vk::BufferUsageFlags Buffer::getUsageFlags(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::VERTEX_BUFFER: return vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        case BufferUsage::TRANSFER_BUFFER: return vk::BufferUsageFlagBits::eTransferSrc;
        case BufferUsage::UNIFORM_BUFFER: return vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
    }

    return {};
}

