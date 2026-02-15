#include "pch.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/Renderer-Exceptions.hpp"
#include <vma/vk_mem_alloc.h>
#include "Utils.hpp"

namespace InitialValues {
    constexpr vk::Extent2D windowSize = {
        .width = 800,
        .height = 600
    };
}

void Renderer::InitGLFW(const std::string& title) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(InitialValues::windowSize.width, InitialValues::windowSize.height, title.c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, FrameBufferSizeCallback);
}

void Renderer::FrameBufferSizeCallback(GLFWwindow *window, int, int) {
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->m_frameBufferResized = true; 
}

Renderer::InitResult Renderer::Init(const std::string& title) {
    InitGLFW(title);

    try {
        CreateInstance(title);
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDeviceAndQueues();
        CreateSwapChain();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
        CreateAllocator();
    }
    catch (const CreateInstance_Error& e) {
        DEBUG_PRINT(e.what()); 
        return InitResult::INSTANCE_FAILED;
    }
    catch (const CreateSurface_Error& e) {
        DEBUG_PRINT(e.what()); 
        return InitResult::SURFACE_FAILED;
    }
    catch (const PickPhysicalDevice_Error& e) {
        DEBUG_PRINT(e.what()); 
        return InitResult::PICK_PHYSICAL_DEVICE_FAILED;
    }
    catch (const CreateLogicalDevice_Error& e) {
        DEBUG_PRINT(e.what()); 
        return InitResult::LOGICAL_DEVICE_FAILED;
    }
    catch (const CreateAllocatorError& e) {
        DEBUG_PRINT(e.what()); 
        return InitResult::ALLOCATOR_FAILED;
    }

    return InitResult::OK;
}

void Renderer::Update() {
    glfwPollEvents();
}

void Renderer::Shutdown() {
    m_device.waitIdle();

    for (const std::shared_ptr<Buffer>& buffer : m_buffers) {
        vmaDestroyBuffer(m_allocator, buffer->buffer, buffer->allocation);
    }

    if (m_allocator) {
        vmaDestroyAllocator(m_allocator);
    }

    CleanupSwapChain();

    glfwDestroyWindow(m_window);

    glfwTerminate();
}

bool Renderer::isRunning() const {
    return !glfwWindowShouldClose(m_window);
}
