cmake_minimum_required(VERSION 3.21)
project(Renderer LANGUAGES CXX)

# ---- Language ----
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Executable ----
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/pch.cpp
    src/Renderer/Renderer-core.cpp
    src/Renderer/Renderer-Instance.cpp
    src/Renderer/Renderer-Debugger.cpp
    src/Renderer/Renderer-Surface.cpp
    src/Renderer/Renderer-PhysicalDevice.cpp
    src/Renderer/Renderer-LogicalDevice.cpp
    src/Renderer/Renderer-SwapChain.cpp
    src/Renderer/Renderer-CommandBuffers.cpp
    src/Renderer/Renderer-Renderer.cpp
    src/Renderer/Renderer-RenderGraph.cpp
    src/stbImplementation/stbImplementation.cpp
)

# Precompiled header
target_precompile_headers(${PROJECT_NAME} PRIVATE include/pch.hpp)

# ---- Vulkan ----
find_package(Vulkan REQUIRED)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Vulkan::Vulkan
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/include
            ${Vulkan_INCLUDE_DIRS}
            ${glfw_SOURCE_DIR}/include
            ${glm_SOURCE_DIR}
)

# ---- FetchContent ----
include(FetchContent)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
)
FetchContent_MakeAvailable(glm)

target_link_libraries(${PROJECT_NAME}
    PRIVATE glfw glm
)

# ---- MinGW runtime DLL copy (Windows only) ----
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    foreach(dll
        libstdc++-6.dll
        libgcc_s_seh-1.dll
        libwinpthread-1.dll
    )
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${dll}
            OUTPUT_VARIABLE DLL_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(EXISTS ${DLL_PATH})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_PATH}"
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endif()
    endforeach()
endif()

