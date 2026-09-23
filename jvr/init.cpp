// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "init.hpp"
#include <iostream>
#include <optional>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_funcs.hpp>

inline bool operator!(vk::Result result)
{
    return result != vk::Result::eSuccess;
}

#define THROW(msg)                                          \
    throw std::runtime_error(msg "\n" __FILE__ ":" +        \
                             std::to_string(__LINE__ - 3) + \
                             "\nCheck this.result to know the error")

VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

inline vk::Result createDebugUtilsMessenger(
    jvr::setup *setup, const vk::DebugUtilsMessengerCreateInfoEXT *pCreateInfo)
{
    using jvr_PFN_vkCreateDebugUtilsMessengerEXT = vk::Result (*)(vk::Instance,
        const vk::DebugUtilsMessengerCreateInfoEXT *, vk::AllocationCallbacks *,
        vk::DebugUtilsMessengerEXT *);
    PFN_vkCreateDebugUtilsMessengerEXT f;
    auto func =
        (jvr_PFN_vkCreateDebugUtilsMessengerEXT)setup->m_instance.getProcAddr(
            "vkCreateDebugUtilsMessengerEXT", setup->m_dispatch_loader);
    if (func != nullptr) {
        return func(setup->m_instance, pCreateInfo, setup->m_alloca_callback,
            &setup->debugMessenger);
    } else {
        return vk::Result::eErrorExtensionNotPresent;
    }
};

void jvr::setup::setupDebugMessenger()
{
    if (!check_validation)
        return;

    vk::DebugUtilsMessengerCreateInfoEXT createInfo = {};
    populateDebugMessengerCreateInfo(createInfo);

    result = createDebugUtilsMessenger(this, &createInfo);

    if (!result) {
        THROW("failed to set up debug messenger");
    }
}

bool jvr::setup::checkValidationLayers()
{
    result = vk::enumerateInstanceLayerProperties(
        &m_layerProps.len, VK_NULL_HANDLE, m_dispatch_loader);
    if (!result) {
        THROW("Failed to get layer properties");
    }

    m_layerProps.capacity_from_len();
    result = vk::enumerateInstanceLayerProperties(
        &m_layerProps.len, m_layerProps.data, m_dispatch_loader);
    if (!result) {
        THROW("Failed to get layer properties");
    }

    std::cout << "VkLayerProperties (" << m_layerProps.len << "):\n";

    for (VkLayerProperties layer : m_layerProps) {
        char specVersion[20] = "";

        sprintf(specVersion, "%d.%d.%d",
            VK_API_VERSION_MAJOR(layer.specVersion),
            VK_API_VERSION_MINOR(layer.specVersion),
            VK_API_VERSION_PATCH(layer.specVersion));
        std::cout << "\t" << layer.layerName << "(" << specVersion << ", "
                  << layer.implementationVersion << "): " << layer.description
                  << "\n";
    }

    for (VkLayerProperties layer : m_layerProps) {
        if (!strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation")) {
            return true;
        }
    }
    return false;
}

void jvr::setup::createInstance()
{
    if (check_validation && !checkValidationLayers()) {
        throw std::runtime_error(
            "validation layers requested, but not available!");
    }
    vk::ApplicationInfo app_info{};
    app_info.pApplicationName = "Vulkan App";
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 3, 0);
    app_info.applicationVersion = VK_MAKE_VERSION(1, 3, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;
    app_info.sType = vk::StructureType::eApplicationInfo;

    vk::InstanceCreateInfo instanceInfo{};
    instanceInfo.pApplicationInfo = &app_info;
    instanceInfo.sType = vk::StructureType::eInstanceCreateInfo;

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (check_validation) {
        instanceInfo.enabledLayerCount = 1;
        instanceInfo.ppEnabledLayerNames = (char **)&m_validationLayers;

        populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceInfo.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.pNext = VK_NULL_HANDLE;
    }

    loadGlfwExtensions();

    m_glfwRequiredExtensions.push(m_validationGlfwExtensions[0]);
    if (check_validation)
        m_glfwRequiredExtensions.push(m_validationGlfwExtensions[1]);

    instanceInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
    instanceInfo.enabledExtensionCount = m_glfwRequiredExtensions.len;
    instanceInfo.ppEnabledExtensionNames = m_glfwRequiredExtensions.data;

    result = vk::createInstance(
        &instanceInfo, m_alloca_callback, &m_instance, m_dispatch_loader);
    if (!result) {
        THROW("failed to create Instance");
    }
}

void jvr::setup::loadGlfwExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    m_glfwRequiredExtensions = glfwExtensionCount;

    std::cout << "GLFWRequiredExtensions (" << glfwExtensionCount << "):\n";
    for (int i = 0; i < glfwExtensionCount; i++) {
        m_glfwRequiredExtensions[i] = glfwExtensions[i];
        std::cout << "\t " << glfwExtensions[i] << "\n";
    }
}

void jvr::setup::loadVkExtensions()
{
    result = vk::enumerateInstanceExtensionProperties(
        nullptr, &m_vkExtensions.len, VK_NULL_HANDLE, m_dispatch_loader);
    if (!result) {
        THROW("failed to get extensions count");
    }

    m_vkExtensions.capacity_from_len();

    result = vk::enumerateInstanceExtensionProperties(
        nullptr, &m_vkExtensions.len, m_vkExtensions.data, m_dispatch_loader);
    if (result != vk::Result::eSuccess) {
        THROW("failed to get extensions");
    }

    std::cout << "VkExtensionProperties (" << m_vkExtensions.len << "):\n";
    for (uint32_t i = 0; i < m_vkExtensions.len; i++) {
        std::cout << "\t " << m_vkExtensions[i].extensionName << " ("
                  << m_vkExtensions[i].specVersion << ")\n";
    }
}

void jvr::setup::loadGPU()
{
    array<vk::PhysicalDevice> s_devices;
    result = m_instance.enumeratePhysicalDevices(
        &s_devices.len, nullptr, m_dispatch_loader);
    if (!result) {
        THROW("failed to get physical devices count");
    }
    s_devices.capacity_from_len();
    result = m_instance.enumeratePhysicalDevices(
        &s_devices.len, s_devices.data, m_dispatch_loader);
    if (!result) {
        THROW("failed to get physical devices");
    }

    for (vk::PhysicalDevice device : s_devices) {
        vk::PhysicalDeviceProperties deviceProperties;
        vk::PhysicalDeviceFeatures deviceFeatures;
        device.getProperties(&deviceProperties, m_dispatch_loader);
        device.getFeatures(&deviceFeatures, m_dispatch_loader);

        if (deviceFeatures.geometryShader &&
            (deviceProperties.deviceType ==
                    vk::PhysicalDeviceType::eDiscreteGpu ||
                deviceProperties.deviceType ==
                    vk::PhysicalDeviceType::eIntegratedGpu)) {
            array<vk::QueueFamilyProperties> s_device_queue_props;
            device.getQueueFamilyProperties(
                &s_device_queue_props.len, VK_NULL_HANDLE);
            s_device_queue_props.capacity_from_len();
            device.getQueueFamilyProperties(
                &s_device_queue_props.len, s_device_queue_props.data);

            bool graphics = false;
            bool compute = false;
            std::optional<uint32_t> graphical_index;
            std::optional<uint32_t> present_index;

            for (int i = 0; i < s_device_queue_props.len; i++) {
                auto props = s_device_queue_props[i];
                if (props.queueFlags & vk::QueueFlagBits::eCompute) {
                    compute = true;
                }
                if (props.queueFlags & vk::QueueFlagBits::eGraphics) {
                    graphics = true;
                    graphical_index = i;
                }
                VkBool32 presentSupport = false;
                result =
                    device.getSurfaceSupportKHR(i, m_surface, &presentSupport);
                if (!result) {
                    THROW("failed to get surface support");
                }

                if (presentSupport) {
                    present_index = i;
                }
            }
            if (graphics && compute && present_index.has_value()) {
                m_gpuDevice = device;
                indices = static_cast<uint64_t>(graphical_index.value()) << 32 |
                          present_index.value();
                break;
            }
        }
    }

    if (m_gpuDevice != VK_NULL_HANDLE) {
        vk::PhysicalDeviceProperties deviceProperties;
        m_gpuDevice.getProperties(&deviceProperties);

        std::cout << "Available GPU:\n\t Name: " << deviceProperties.deviceName
                  << "\n\t Type: ";
        if (deviceProperties.deviceType ==
            vk::PhysicalDeviceType::eDiscreteGpu) {
            std::cout << "Discrete GPU";
        } else
            std::cout << "Integrated GPU";
        std::flush(std::cout);

        std::cout << "\n\t Device ID: 0x" << std::hex
                  << deviceProperties.deviceID << "\n\t Vendor ID: 0x"
                  << deviceProperties.vendorID << std::dec
                  << "\n\t Driver Version: ";

        printf("\n\t API Version: %d.%d.%d",
            VK_API_VERSION_MAJOR(deviceProperties.apiVersion),
            VK_API_VERSION_MINOR(deviceProperties.apiVersion),
            VK_API_VERSION_PATCH(deviceProperties.apiVersion));

        printf("\n\t Driver Version: %d.%d.%d\n",
            VK_API_VERSION_MAJOR(deviceProperties.driverVersion),
            VK_API_VERSION_MINOR(deviceProperties.driverVersion),
            VK_API_VERSION_PATCH(deviceProperties.driverVersion));

        fflush(stdout);
    }
}
