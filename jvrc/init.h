// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#ifndef JVR_INIT_H
#define JVR_INIT_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "array.h"

typedef struct jvr_setup {
    GLFWwindow *m_window;
    VkInstance m_instance;
    char *validationLayers[1]; // NOTE: Free [0] in render::destroy()
    char *m_validationGlfwExtensions[2];
    char check_validation;
    jvr_array m_glfwRequiredExtensions;
    jvr_array m_vkExtensions;
    jvr_array m_layerProps;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice m_gpuDevice;
    VkDevice m_logicalDevice;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR m_surface;
    uint64_t indices;

    VkResult result;
} jvr_setup;

void jvr_setup_new(jvr_setup *setup);

void jvr_setup_init(jvr_setup *setup);

void jvr_setup_createInstance(jvr_setup *setup);
char jvr_setup_checkValidationLayers(jvr_setup *setup);
void jvr_setup_setupDebugMessenger(jvr_setup *setup);
void jvr_setup_loadGlfwExtensions(jvr_setup *setup);
void jvr_setup_createSurface(jvr_setup *setup);
void jvr_setup_loadVkExtensions(jvr_setup *setup);
void jvr_setup_loadGPU(jvr_setup *setup);
void jvr_setup_createLogicalDevice(jvr_setup *setup);

#endif // !__VK_RENDER_INIT__H
