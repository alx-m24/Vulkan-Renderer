#pragma once

enum class BufferUsage {
    VERTEX_BUFFER,
    TRANSFER_BUFFER,
    UNIFORM_BUFFER
};

struct BufferDescription {
    std::string name = "";
    size_t size = {};
    BufferUsage usage = {};
};
