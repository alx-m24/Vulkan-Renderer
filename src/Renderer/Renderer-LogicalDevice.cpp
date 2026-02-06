#include "pch.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/Renderer-Exceptions.hpp"

static bool QueueSupportsPresent(Renderer::QueueFamilyIndex index, const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) {
    return device.getSurfaceSupportKHR(index, surface);
}

static Renderer::QueueFamilyIndex getGraphicsQueueFamilyIndex(const std::vector<vk::QueueFamilyProperties>& queueFamilyProperties) {
    for (size_t i{0}; i < queueFamilyProperties.size(); ++i) {
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            return i;
        }
    }

    throw CreateLogicalDevice_Error("Failed to get queue supporting graphics");
}

void Renderer::CreateLogicalDeviceAndQueues() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();

    QueueFamilyIndex graphicsIndex = getGraphicsQueueFamilyIndex(queueFamilyProperties);

    std::optional<QueueFamilyIndex> presentIndex;

    bool graphicsQueueSupportsPresent = QueueSupportsPresent(graphicsIndex, m_physicalDevice, m_surface);

    if (graphicsQueueSupportsPresent) {
        presentIndex = graphicsIndex;
    }
    else {
        // IF graphics queue does not support present
        // find queue that supports both graphics and present
        for (size_t i{0}; i < queueFamilyProperties.size() && !graphicsQueueSupportsPresent; ++i) {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && QueueSupportsPresent(i, m_physicalDevice, m_surface)) {
                graphicsIndex = i;
                presentIndex = graphicsIndex;
                graphicsQueueSupportsPresent = true;
                break;
            }
        }

        // IF NO queue supporting both present and graphics
        // Find first OTHER queue supporting present
        for (size_t i{0}; i < queueFamilyProperties.size() && !graphicsQueueSupportsPresent; ++i) {
            if (QueueSupportsPresent(i, m_physicalDevice, m_surface)) {
                presentIndex = i;
                break;
            }
        }
    }

    if (!presentIndex.has_value()) {
        throw CreateLogicalDevice_Error("Could not find a queue for present");
    }

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(2);

    float queuePriority = 0.5f;

    queueCreateInfos.emplace_back(
        vk::DeviceQueueCreateInfo {
            .queueFamilyIndex = graphicsIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        }
    );

    if (presentIndex.value() != graphicsIndex) {
        queueCreateInfos.emplace_back(
            vk::DeviceQueueCreateInfo {
                .queueFamilyIndex = presentIndex.value(),
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
            }
        );
    }

    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {},
        { .shaderDrawParameters = true },
        { .synchronization2 = true, .dynamicRendering = true },
        { .extendedDynamicState = true }
    };

    std::vector<const char*> requiredExtensionsNames;
    requiredExtensionsNames.reserve(DeviceExtensions::requiredExtensions.size());
    for (const DeviceExtensions::DeviceExtension& extensions : DeviceExtensions::requiredExtensions) {
        requiredExtensionsNames.push_back(extensions.name);
    }

    vk::DeviceCreateInfo deviceCreateInfo {
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensionsNames.size()),
        .ppEnabledExtensionNames = requiredExtensionsNames.data()
    };

    m_device = vk::raii::Device(m_physicalDevice, deviceCreateInfo);

    // Getting queues
    m_graphicsQueue = vk::raii::Queue(m_device, graphicsIndex, 0);
    m_graphicsFamilyIndex = graphicsIndex;

    m_presentQueue  = vk::raii::Queue(m_device, presentIndex.value(), 0);
    m_presentFamilyIndex = presentIndex.value();

    // TODO: FIND OTHER QUEUES FOR TRANSFER
    m_transferQueue = m_graphicsQueue;
    m_transferFamilyIndex = m_graphicsFamilyIndex;
}
