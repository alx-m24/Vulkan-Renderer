#pragma once

enum class VertexInputRate {
    PER_VERTEX = 0,
    PER_INSTANCE
};

enum class VertexAttributeType {
    FLOAT = 0,
    VEC_2,
    VEC_3,
    VEC_4,
    
    INT,
    IVEC_2,
    IVEC_3,
    IVEC_4,

    UINT,
    UVEC_2,
    UVEC_3,
    UVEC_4,

    BYTE_4_NORM,
    SHORT_2_NORM
};

struct VertexBindingDescription {
    uint32_t binding = 0;
    uint32_t stride = 0;
    VertexInputRate inputRate = VertexInputRate::PER_VERTEX;
};

struct VertexAttributeDescription {
    uint32_t location = 0;
    uint32_t binding = 0;
    VertexAttributeType type = VertexAttributeType::FLOAT;
    uint32_t offset = 0;
};

struct VertexInfo {
    std::vector<VertexBindingDescription> bindings;
    std::vector<VertexAttributeDescription> attributes;
};
