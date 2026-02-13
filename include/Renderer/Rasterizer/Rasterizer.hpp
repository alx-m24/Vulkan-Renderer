#pragma once

enum class PolygonMode : uint8_t {
    FILL,
    LINE,
    POINT
};

enum class CullFace : uint8_t {
    NONE,
    FRONT,
    BACK
};

enum class FrontFace : uint8_t {
    CW,
    CCW 
};

struct FaceCulling {
    CullFace cullFace = CullFace::NONE;
    FrontFace frontFace = FrontFace::CW;
};

struct Rasterizer {
    bool depthClampEnable = false;
    bool discardEnable = false;
    PolygonMode polygonMode = PolygonMode::FILL;
    FaceCulling faceCulling {};
};

