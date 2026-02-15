#include "pch.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/Renderer-Exceptions.hpp"
#include <vma/vk_mem_alloc.h>

void Renderer::CreateAllocator() {
    VmaAllocatorCreateInfo createInfo{};
    createInfo.device = *m_device;
    createInfo.physicalDevice = *m_physicalDevice;
    createInfo.instance = *m_instance;
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    VmaAllocator allocator{};
    VkResult result = vmaCreateAllocator(&createInfo, &allocator);
    if (result != VK_SUCCESS) {
        throw CreateAllocatorError("Failed to create VMA allocator");
    }

    m_allocator = allocator;
}
