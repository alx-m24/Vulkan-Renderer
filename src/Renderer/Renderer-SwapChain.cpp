#include "pch.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/RenderGraph/RenderGraph.hpp"
#include "Renderer/Renderer-Exceptions.hpp"

static vk::Extent2D ChooseSwapChainExtent(GLFWwindow* const window, const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) {
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return surfaceCapabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {
        std::clamp<uint32_t>(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
    };
}

static vk::SurfaceFormatKHR ChooseSwapChainFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    if (availableFormats.empty()) {
        throw CreateSwapChain_Error("No available formats");
    }

    const auto formatItr = std::ranges::find_if(availableFormats,
            [] (const vk::SurfaceFormatKHR& format) -> bool {
                return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
            });
    return formatItr == availableFormats.end() ? availableFormats.front() : *formatItr;
}

static vk::PresentModeKHR ChooseSwapChainPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    if (availablePresentModes.empty()) {
        throw CreateSwapChain_Error("No available present modes");
    }

    return std::ranges::any_of(availablePresentModes,
            [] (const vk::PresentModeKHR& value) -> bool { 
                return vk::PresentModeKHR::eMailbox == value; 
            }) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
}

static uint32_t choooseMinSwapChainImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) {
    uint32_t minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if (minImageCount >= surfaceCapabilities.maxImageCount) {
        return surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

void Renderer::CreateSwapChain() {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);

    vk::Extent2D extent = ChooseSwapChainExtent(m_window, surfaceCapabilities);
    vk::SurfaceFormatKHR format = ChooseSwapChainFormat(m_physicalDevice.getSurfaceFormatsKHR(*m_surface));
    vk::PresentModeKHR presentMode = ChooseSwapChainPresentMode(m_physicalDevice.getSurfacePresentModesKHR(*m_surface));

    vk::SwapchainCreateInfoKHR swapChainCreateInfo {
        .surface = *m_surface,
        .minImageCount = choooseMinSwapChainImageCount(surfaceCapabilities),
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = true,
        .oldSwapchain = nullptr
    };

    m_swapChainExtent = extent;
    m_SwapChainSurfaceFormat = format;

    m_swapChain = vk::raii::SwapchainKHR(m_device, swapChainCreateInfo);

    m_swapChainImages.clear();
    m_swapChainImageViews.clear();

    m_swapChainImages = m_swapChain.getImages();
    m_swapChainImageViews.reserve(m_swapChainImages.size());
   
    vk::ImageViewCreateInfo imageViewCreateInfo {
        .viewType = vk::ImageViewType::e2D,
        .format = format.format,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };
    for (const vk::Image& image : m_swapChainImages) {
        imageViewCreateInfo.image = image;
        m_swapChainImageViews.emplace_back(m_device, imageViewCreateInfo);
    };
}

void Renderer::reCreateSwapChain() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	m_device.waitIdle();

	CleanupSwapChain();
	CreateSwapChain();

    ImageResource& backBuffer = m_renderGraph->getResourceUnsafe("BackBuffer");
    backBuffer.format = m_SwapChainSurfaceFormat.format;
    backBuffer.extent = m_swapChainExtent;
}

void Renderer::CleanupSwapChain() {
	m_swapChainImageViews.clear();
	m_swapChain = nullptr;
}
