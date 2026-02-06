#include "pch.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/Renderer-Exceptions.hpp"

#include "Utils.hpp"

static std::optional<vk::raii::PhysicalDevice> getSuitableDevice(const std::vector<vk::raii::PhysicalDevice>& availableDevices) {
    auto deviceItr = std::ranges::find_if(availableDevices,
            [] (const vk::raii::PhysicalDevice& candidate) {
                bool supportsVK_1_3 = candidate.getProperties().apiVersion >= VK_API_VERSION_1_3;
                if (!supportsVK_1_3) {
                    return false;
                }

                std::vector<vk::QueueFamilyProperties> queueFamilies = candidate.getQueueFamilyProperties();
                bool anyGraphicsQueues = std::ranges::any_of(queueFamilies, 
                        [] (const vk::QueueFamilyProperties& queueProperties) -> bool {
                            return bool(queueProperties.queueFlags & vk::QueueFlagBits::eGraphics);
                        });
                if (!anyGraphicsQueues) {
                    return false;
                }

                bool allExtensionsSupported = isAllPresent(DeviceExtensions::requiredExtensions, candidate.enumerateDeviceExtensionProperties());
                if (!allExtensionsSupported) {
                    return false;
                }

                auto features = candidate.getFeatures2<vk::PhysicalDeviceFeatures2,
                                            vk::PhysicalDeviceVulkan13Features,
                                            vk::PhysicalDeviceVulkan11Features,
                                            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
                bool allFeaturesRequired = features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                                            features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                            features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

                return allFeaturesRequired;
            });

    if (deviceItr == availableDevices.end()) {
        return std::optional<vk::raii::PhysicalDevice> {};
    }

    return *deviceItr;
}

void Renderer::PickPhysicalDevice() {
    std::vector<vk::raii::PhysicalDevice> availableDevices = m_instance.enumeratePhysicalDevices();

    std::optional<vk::raii::PhysicalDevice> suitableDevice = getSuitableDevice(availableDevices);
    if (!suitableDevice.has_value()) {
        throw PickPhysicalDevice_Error("No suitable physical device");
    }

    m_physicalDevice = std::move(suitableDevice.value());
}
