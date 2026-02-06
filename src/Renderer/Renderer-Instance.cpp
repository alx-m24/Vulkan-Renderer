#include "pch.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/Renderer-Exceptions.hpp"

#include "Utils.hpp"

namespace ValidationLayers {
    struct ValidationLayer {
        const char* name;

        bool operator==(const vk::LayerProperties& layerProperties) const {
            return std::strcmp(name, layerProperties.layerName) == 0;
        }
    };

#ifndef NDEBUG
    constexpr bool enabled = true;
    const std::vector<ValidationLayer> validationLayers = {
        ValidationLayer{ "VK_LAYER_KHRONOS_validation" }
    };
#else
    constexpr bool enabled = false;
    const std::vector<ValidationLayer> validationLayers;
#endif
}

namespace Extensions {
    struct Extension {
        const char* name;
    
        bool operator==(const vk::ExtensionProperties& extensionProperties) const {
            return std::strcmp(name, extensionProperties.extensionName) == 0;
        }
    };
}

void Renderer::CreateInstance(const std::string& title) {
    const vk::ApplicationInfo appInfo {
        .pApplicationName = title.c_str(),
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion13,
    };

    // Query validation layers 
    std::vector<const char*> layerNames;
    if constexpr (ValidationLayers::enabled) {
        std::vector<vk::LayerProperties> layersProperties = m_context.enumerateInstanceLayerProperties();

        // If ANY Validation layer is NOT found in layerProperties -> throw
        if (!isAllPresent(ValidationLayers::validationLayers, layersProperties)) {
            throw CreateInstance_Error("One or more Validation Layer not found");
        }

            layerNames.reserve(ValidationLayers::validationLayers.size());
            for (const auto& layer : ValidationLayers::validationLayers) {
                layerNames.push_back(layer.name);
            }
    }

    // Query glfw required extensions
    std::vector<Extensions::Extension> requiredExtensions = getRequiredExtensions();
    std::vector<const char*> extensionsNames;
    if (!isAllPresent(requiredExtensions, m_context.enumerateInstanceExtensionProperties())) {
        throw CreateInstance_Error("One or more Required Extensions not supported");
    }
    extensionsNames.reserve(requiredExtensions.size());
    for (const auto& extension : requiredExtensions) {
        extensionsNames.push_back(extension.name);
    }

    vk::InstanceCreateInfo instanceCreateInfo {
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layerNames.size()),
        .ppEnabledLayerNames = layerNames.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensionsNames.size()),
        .ppEnabledExtensionNames = extensionsNames.data(),
    };
    m_instance = vk::raii::Instance(m_context, instanceCreateInfo);

    if constexpr (ValidationLayers::enabled) SetupDebugMessenger();
}

std::vector<Extensions::Extension> Renderer::getRequiredExtensions() const {
   	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<Extensions::Extension> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if constexpr (ValidationLayers::enabled)
	{
		extensions.push_back(Extensions::Extension{ vk::EXTDebugUtilsExtensionName });
	}

    return extensions;
}
