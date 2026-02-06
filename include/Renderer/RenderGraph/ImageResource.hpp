#pragma once

#include <type_traits>
#include <string_view>

struct ImageResource {
    std::string name = "";
    vk::Format format {};                    // Pixel format (RGBA8, Depth24Stencil8, etc.)
    vk::Extent2D extent {};                  // Dimensions in pixels for 2D resources
    vk::ImageUsageFlags usage {};            // How this resource will be used (color attachment, texture, etc.)
    vk::ImageLayout initialLayout {};        // Expected layout when the frame begins
    vk::ImageLayout finalLayout {};          // Required layout when the frame ends

    // Actual GPU resources - populated during compilation
    vk::Image image = nullptr;      // The GPU image object
    vk::raii::DeviceMemory memory = nullptr;  // Backing memory allocation
    vk::ImageView view = nullptr;   // Shader-accessible view of the image
    
    bool operator==(const ImageResource& ir) const {
        return name == ir.name;
    }
};

inline vk::ImageLayout pickLayout(const ImageResource& res, bool isWrite) {
    if (isWrite) {
        if (res.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment)
            return vk::ImageLayout::eDepthStencilAttachmentOptimal;

        return vk::ImageLayout::eColorAttachmentOptimal;
    }

    // Read-only
    if (res.usage & vk::ImageUsageFlagBits::eSampled)
        return vk::ImageLayout::eShaderReadOnlyOptimal;

    return res.initialLayout; // no-op fallback
}
