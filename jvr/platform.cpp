// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "platform.h"

#define THROW(msg) \
    throw std::runtime_error(msg "\nCheck this.result to know the error")

#include <vulkan/vulkan.h>
#if defined VK_PLATFORM_win
#define GLFW_EXPOSE_NATIVE_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan_win32.h>
#elif defined VK_PLATFORM_wayland
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan_wayland.h>
#elif defined VK_PLATFORM_xlib
#define GLFW_EXPOSE_NATIVE_X11
#define VK_USE_PLATFORM_XLIB_KHR
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#elif defined VK_PLATFORM_xcb
#define GLFW_EXPOSE_NATIVE_X11
#define VK_USE_PLATFORM_XCB_KHR
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#endif // !_WIN32
#include "init.hpp"
#include <GLFW/glfw3native.h>

void jvr::setup::createSurface()
{
#ifdef VK_PLATFORM_win
    vk::Win32SurfaceCreateInfoKHR createInfo;
    createInfo.sType = vk::StructureType::eWin32SurfaceCreateInfoKHR;
    createInfo.hwnd = glfwGetWin32Window(m_window);
    createInfo.hinstance = GetModuleHandle(nullptr);
    result = m_instance.createWin32SurfaceKHR(
        &createInfo, m_alloca_callback, &m_surface, m_dispatch_loader);

#elif defined VK_PLATFORM_wayland
    vk::WaylandSurfaceCreateInfoKHR surface_create_info;
    surface_create_info.sType = vk::StructureType::eWaylandSurfaceCreateInfoKHR;
    surface_create_info.surface = glfwGetWaylandWindow(m_window);
    surface_create_info.display = glfwGetWaylandDisplay();

    result = m_instance.createWaylandSurfaceKHR(
        &surface_create_info, m_alloca_callback, &m_surface, m_dispatch_loader);

#elif defined VK_PLATFORM_xlib
    vk::XlibSurfaceCreateInfoKHR surface_create_info{};
    surface_create_info.sType = vk::StructureType::eXlibSurfaceCreateInfoKHR;
    surface_create_info.dpy = glfwGetX11Display();
    surface_create_info.window = glfwGetX11Window(m_window);

    result = m_instance.createXlibSurfaceKHR(
        &surface_create_info, m_alloca_callback, &m_surface, m_dispatch_loader);

#elif defined VK_PLATFORM_xcb
    vk::XcbSurfaceCreateInfoKHR surface_create_info{};
    surface_create_info.sType = vk::StructureType::eXcbSurfaceCreateInfoKHR;
    surface_create_info.connection = xcb_connect(NULL, NULL);
    surface_create_info.window = glfwGetX11Window(m_window);

    result = m_instance.createXcbSurfaceKHR(
        &surface_create_info, m_alloca_callback, &m_surface, m_dispatch_loader);

#else
    result = (vk::Result)glfwCreateWindowSurface(
        (VkInstance)m_instance, m_window, nullptr, (VkSurfaceKHR *)&m_surface);
#endif
    if (result != vk::Result::eSuccess) {
        THROW("failed to create window surface!");
    }
}
