// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "init.h"
#include "array.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vulkan/vk_enum_string_helper.h>

#define THROW(msg, res)                                                     \
    printf(msg "\n" __FILE__ ":%d\nCheck this.result to know the error %s", \
        __LINE__ - 3, string_VkResult(res));                                \
    exit(1);

void jvr_setup_new(jvr_setup *setup)
{
    char *ptr = calloc(1, 30);
    strcpy(ptr, "VK_LAYER_KHRONOS_validation");
    setup->validationLayers[0] = ptr;

    jvr_array_new(&setup->m_glfwRequiredExtensions, sizeof(const char *), 0);
    jvr_array_new(&setup->m_vkExtensions, sizeof(VkExtensionProperties), 0);
    jvr_array_new(&setup->m_layerProps, sizeof(VkLayerProperties), 0);

    setup->m_validationGlfwExtensions[0] =
        malloc(sizeof(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME));
    setup->m_validationGlfwExtensions[1] =
        malloc(sizeof(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));

    strcpy(setup->m_validationGlfwExtensions[0],
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    strcpy(setup->m_validationGlfwExtensions[1],
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}
void jvr_setup_init(jvr_setup *setup)
{
    jvr_setup_createInstance(setup);
    jvr_setup_loadVkExtensions(setup);
    if (setup->check_validation)
        jvr_setup_setupDebugMessenger(setup);

    jvr_setup_createSurface(setup);
    jvr_setup_loadGPU(setup);
    jvr_setup_createLogicalDevice(setup);
}

static VkBool32 debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);

    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT *createInfo)
{
    memset(createInfo, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT));
    createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = debugCallback;
}

extern inline VkResult createDebugUtilsMessenger(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL) {
        return func(instance, pCreateInfo, NULL, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
};

void jvr_setup_setupDebugMessenger(jvr_setup *setup)
{
    if (!setup->check_validation)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = { 0 };
    populateDebugMessengerCreateInfo(&createInfo);

    setup->result = createDebugUtilsMessenger(
        setup->m_instance, &createInfo, &setup->debugMessenger);
    if (setup->result != VK_SUCCESS) {
        THROW("failed to set up debug messenger", setup->result);
    }
}

char jvr_setup_checkValidationLayers(jvr_setup *setup)
{
    vkEnumerateInstanceLayerProperties(
        &setup->m_layerProps.len, VK_NULL_HANDLE);

    jvr_array_new(&setup->m_layerProps, sizeof(VkLayerProperties),
        setup->m_layerProps.len);
    vkEnumerateInstanceLayerProperties(
        &setup->m_layerProps.len, setup->m_layerProps.data);
    if (setup->result != VK_SUCCESS) {
        THROW("Couldn't get Layers", setup->result);
    }

    printf("VkLayerProperties (%d):\n", setup->m_layerProps.len);
    char layerFound = VK_FALSE;

    FOREACH(setup->m_layerProps, VkLayerProperties, layer)
    {
        printf("\t%s", layer->layerName);
        printf("(%d.%d.%d", layer->specVersion >> (6 + 16),
            layer->specVersion >> 12 & 0xFF, layer->specVersion & 0xFFF);
        printf(", %d): %s\n", layer->implementationVersion, layer->description);

        if (!strcmp(layer->layerName, "VK_LAYER_KHRONOS_validation")) {
            layerFound = VK_TRUE;
            break;
        }
    }
    return layerFound;
}

void jvr_setup_createInstance(jvr_setup *setup)
{
    if (setup->check_validation && !jvr_setup_checkValidationLayers(setup)) {
        THROW("validation layers requested, but not available!", setup->result);
    }
    VkApplicationInfo app_info = { 0 };
    app_info.pApplicationName = "Vulkan App";
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

    VkInstanceCreateInfo instanceInfo = { 0 };
    instanceInfo.pApplicationInfo = &app_info;
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { 0 };
    if (setup->check_validation) {
        instanceInfo.enabledLayerCount = 1;
        instanceInfo.ppEnabledLayerNames =
            (const char **)setup->validationLayers;

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        instanceInfo.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.pNext = VK_NULL_HANDLE;
    }

    jvr_setup_loadGlfwExtensions(setup);

    jvr_array_push(
        &setup->m_glfwRequiredExtensions, setup->m_validationGlfwExtensions);
    if (setup->check_validation)
        jvr_array_push(&setup->m_glfwRequiredExtensions,
            setup->m_validationGlfwExtensions + 1);

    instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    instanceInfo.enabledExtensionCount = setup->m_glfwRequiredExtensions.len;
    instanceInfo.ppEnabledExtensionNames = setup->m_glfwRequiredExtensions.data;

    setup->result =
        vkCreateInstance(&instanceInfo, VK_NULL_HANDLE, &setup->m_instance);
    if (setup->result != VK_SUCCESS) {
        THROW("Couldn't create Instance", setup->result);
    }
}

void jvr_setup_loadGlfwExtensions(jvr_setup *setup)
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    printf("GLFWRequiredExtensions (%d):\n", glfwExtensionCount);
    jvr_array_new(&setup->m_glfwRequiredExtensions, 8, glfwExtensionCount);
    for (int i = 0; i < glfwExtensionCount; i++) {
        JVR_ELEM(setup->m_glfwRequiredExtensions, const char *, i) =
            glfwExtensions[i];
        printf("\t %s\n", glfwExtensions[i]);
    }
}

void jvr_setup_loadVkExtensions(jvr_setup *setup)
{
    setup->result = vkEnumerateInstanceExtensionProperties(
        VK_NULL_HANDLE, &setup->m_vkExtensions.len, VK_NULL_HANDLE);
    if (setup->result != VK_SUCCESS) {
        THROW("Couldn't get extensions", setup->result);
    }
    jvr_array_new(&setup->m_vkExtensions, sizeof(VkExtensionProperties),
        setup->m_vkExtensions.len);

    setup->result = vkEnumerateInstanceExtensionProperties(
        VK_NULL_HANDLE, &setup->m_vkExtensions.len, setup->m_vkExtensions.data);
    if (setup->result != VK_SUCCESS) {
        THROW("Couldn't write extensions", setup->result);
    }

    printf("VkExtensionProperties (%d): ", setup->m_vkExtensions.len);
    for (uint32_t i = 0; i < setup->m_vkExtensions.len; i++) {
        printf("\t %s (%u)\n",
            JVR_ELEM(setup->m_vkExtensions, VkExtensionProperties, i)
                .extensionName,
            JVR_ELEM(setup->m_vkExtensions, VkExtensionProperties, i)
                .specVersion);
    }
}

void jvr_setup_loadGPU(jvr_setup *setup)
{
    jvr_array s_devices = { 0 };
    vkEnumeratePhysicalDevices(
        setup->m_instance, &s_devices.len, VK_NULL_HANDLE);
    jvr_array_new(&s_devices, sizeof(VkPhysicalDevice), s_devices.len);
    vkEnumeratePhysicalDevices(
        setup->m_instance, &s_devices.len, s_devices.data);
    FOREACH(s_devices, VkPhysicalDevice, device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(*device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(*device, &deviceFeatures);

        if (deviceFeatures.geometryShader &&
            (deviceProperties.deviceType ==
                    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
                deviceProperties.deviceType ==
                    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)) {
            jvr_array s_device_queue_props = { 0 };
            vkGetPhysicalDeviceQueueFamilyProperties(
                *device, &s_device_queue_props.len, VK_NULL_HANDLE);
            jvr_array_new(&s_device_queue_props,
                sizeof(VkQueueFamilyProperties), s_device_queue_props.len);
            vkGetPhysicalDeviceQueueFamilyProperties(
                *device, &s_device_queue_props.len, s_device_queue_props.data);

            char graphics = 0;
            char compute = 0;
            uint32_t graphical_index = 0;
            uint32_t present_index = 0;
            for (int i = 0; i < s_device_queue_props.len; i++) {
                VkQueueFamilyProperties props =
                    JVR_ELEM(s_device_queue_props, VkQueueFamilyProperties, i);
                if (props.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    compute = 1;
                }
                if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphics = 1;
                    graphical_index = i;
                }
                VkBool32 presentSupport = 0;
                vkGetPhysicalDeviceSurfaceSupportKHR(
                    *device, i, setup->m_surface, &presentSupport);

                if (presentSupport) {
                    present_index = i;
                }
            }
            if (graphics && compute) {
                setup->m_gpuDevice = *device;
                setup->indices = (uint64_t)graphical_index << 32 |
                                 present_index;
                break;
            }
        }
    }

    if (setup->m_gpuDevice != VK_NULL_HANDLE) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(setup->m_gpuDevice, &deviceProperties);

        printf("Available GPU:\n\t Name: %s\n\t Type: ",
            deviceProperties.deviceName);
        if (deviceProperties.deviceType ==
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            printf("Discrete GPU");
        } else
            printf("Integrated GPU");

        printf("\n\t Device ID: 0x%X\n\t Vendor ID: 0x%X",
            deviceProperties.deviceID, deviceProperties.vendorID);

        printf("\n\t API Version: %d.%d.%d",
            deviceProperties.apiVersion >> (5 * 4 + 2),
            (deviceProperties.apiVersion >> 12) & 0xFF,
            deviceProperties.apiVersion & 0xFFF);

        printf("\n\t Driver Version: %d.%d.%d",
            deviceProperties.driverVersion >> (6 + 16),
            (deviceProperties.driverVersion >> 12) & 0xFF,
            deviceProperties.driverVersion & 0xFFF);
        printf("\n");
    }
}

void jvr_setup_createLogicalDevice(jvr_setup *setup)
{
    uint32_t present_index = setup->indices & UINT32_MAX;
    uint32_t graphics_index = setup->indices >> 32;
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo *queue = malloc(sizeof(VkDeviceQueueCreateInfo[2]));
    queue[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue[0].queueFamilyIndex = graphics_index;
    queue[0].queueCount = 1;
    queue[0].pQueuePriorities = &queuePriority;
    queue[0].pNext = VK_NULL_HANDLE;
    queue[0].flags = 0;

    queue[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue[1].queueFamilyIndex = present_index;
    queue[1].queueCount = 1;
    queue[1].pQueuePriorities = &queuePriority;
    queue[1].pNext = VK_NULL_HANDLE;
    queue[1].flags = 0;

    VkPhysicalDeviceFeatures deviceFeatures = { 0 };
    VkDeviceCreateInfo createInfo = { 0 };

    const char *deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queue;
    createInfo.queueCreateInfoCount = graphics_index != present_index ? 2 : 1;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = 1;

    if (setup->check_validation) {
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = (const char **)setup->validationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }
    setup->result = vkCreateDevice(setup->m_gpuDevice, &createInfo,
        VK_NULL_HANDLE, &setup->m_logicalDevice);
    if (setup->result != VK_SUCCESS) {
        THROW("failed to create logical device!", setup->result);
    }

    vkGetDeviceQueue(
        setup->m_logicalDevice, graphics_index, 0, &setup->graphicsQueue);
    vkGetDeviceQueue(
        setup->m_logicalDevice, present_index, 0, &setup->presentQueue);
}
