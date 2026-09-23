// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#ifndef JVR_INIT_H
#define JVR_INIT_H

#include <iostream>
#include <optional>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define VK_STATIC_LOADER
#include <vulkan/vulkan.hpp>
#include "array.hpp"

namespace jvr
{
struct setup {
    GLFWwindow *m_window;
    vk::DispatchLoaderStatic m_dispatch_loader;
    vk::AllocationCallbacks *m_alloca_callback = VK_NULL_HANDLE;
    vk::Instance m_instance;
    char *m_validationLayers;
    char m_validationGlfwExtensions[2][35] = {
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    bool check_validation = false;
    jvr::array<const char *> m_glfwRequiredExtensions;
    jvr::array<vk::ExtensionProperties> m_vkExtensions;
    jvr::array<vk::LayerProperties> m_layerProps;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::SurfaceKHR m_surface;
    vk::PhysicalDevice m_gpuDevice;
    std::optional<uint64_t> indices;

    vk::Result result = vk::Result::eSuccess;

    static void glfw_error_callback(int error, const char *description)
    {
        std::cerr << "Glfw Error " << error << description << "\n";
    }
    void createInstance();
    bool checkValidationLayers();
    void setupDebugMessenger();
    void loadGlfwExtensions();
    void createSurface();
    void loadVkExtensions();
    void loadGPU();
    uint32_t findMemoryType(
        uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_gpuDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) ==
                    properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    setup()
    {
        m_dispatch_loader = vk::getDispatchLoaderStatic();
        m_validationLayers = new char[30];
        strcpy(m_validationLayers, "VK_LAYER_KHRONOS_validation");
    }

    void init(const char *app_title, int width, int height)
    {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) {
            throw std::runtime_error("Can't initialize GLFW");
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(width, height, app_title, NULL, NULL);
        if (m_window == NULL) {
            std::runtime_error("Can't create GLFW window");
        }
        createInstance();
        loadVkExtensions();
        if (check_validation)
            setupDebugMessenger();

        createSurface();
        loadGPU();
    }

    void destroy()
    {
        m_instance.destroySurfaceKHR(
            m_surface, m_alloca_callback, m_dispatch_loader);
        m_instance.destroy(m_alloca_callback, m_dispatch_loader);
        glfwDestroyWindow(m_window);
        glfwTerminate();
        delete[] m_validationLayers;
    }
};
}

#endif // !JVR_RENDER_INIT__H
