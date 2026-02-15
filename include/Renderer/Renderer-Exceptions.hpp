#pragma once

class CreateInstance_Error : public std::runtime_error {
    public:
        CreateInstance_Error(const std::string& msg) : std::runtime_error(msg) {}
};

class CreateSurface_Error : public std::runtime_error {
    public:
        CreateSurface_Error(const std::string& msg) : std::runtime_error(msg) {}
};

class PickPhysicalDevice_Error : public std::runtime_error {
    public:
        PickPhysicalDevice_Error(const std::string& msg) : std::runtime_error(msg) {}
};

class CreateLogicalDevice_Error : public std::runtime_error {
    public:
        CreateLogicalDevice_Error(const std::string& msg) : std::runtime_error(msg) {}
};

class CreateSwapChain_Error : public std::runtime_error {
    public:
        CreateSwapChain_Error(const std::string& msg) : std::runtime_error(msg) {}
};

class PipelineCreation_Error : public std::runtime_error {
    public:
        PipelineCreation_Error(const std::string& msg) : std::runtime_error(msg) {}
};

class CreateAllocatorError : public std::runtime_error {
    public:
        CreateAllocatorError(const std::string& msg) : std::runtime_error(msg) {}
};
