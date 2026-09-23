// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "init.h"
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vk_enum_string_helper.h>

#define THROW(msg, res)                                                        \
    printf(                                                                    \
        msg "\nCheck this.result to know the error %s", string_VkResult(res)); \
    exit(1);

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
#include <GLFW/glfw3native.h>

void jvr_setup_createSurface(jvr_setup *setup)
{
#ifdef VK_PLATFORM_win
    VkWin32SurfaceCreateInfoKHR createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = glfwGetWin32Window(setup->m_window);
    createInfo.hinstance = GetModuleHandle(VK_NULL_HANDLE);
    setup->result = vkCreateWin32SurfaceKHR(
        setup->m_instance, &createInfo, VK_NULL_HANDLE, &setup->m_surface);

#elif defined VK_PLATFORM_wayland
    VkWaylandSurfaceCreateInfoKHR surface_create_info = { 0 };
    surface_create_info.sType =
        VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    surface_create_info.surface = glfwGetWaylandWindow(setup->m_window);
    surface_create_info.display = glfwGetWaylandDisplay();

    setup->result = vkCreateWaylandSurfaceKHR(
        setup->m_instance, &surface_create_info, NULL, &setup->m_surface);

#elif defined VK_PLATFORM_xlib
    VkXlibSurfaceCreateInfoKHR surface_create_info = { 0 };
    surface_create_info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    surface_create_info.dpy = glfwGetX11Display();
    surface_create_info.window = glfwGetX11Window(setup->m_window);

    setup->result = vkCreateXlibSurfaceKHR(setup->m_instance,
        &surface_create_info, VK_NULL_HANDLE, &setup->m_surface);

#elif defined VK_PLATFORM_xcb
    VkXcbSurfaceCreateInfoKHR surface_create_info = { 0 };
    surface_create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surface_create_info.connection = xcb_connect(NULL, NULL);
    surface_create_info.window = glfwGetX11Window(setup->m_window);

    setup->result = vkCreateXcbSurfaceKHR(setup->m_instance,
        &surface_create_info, VK_NULL_HANDLE, &setup->m_surface);

#else
    glfwCreateWindowSurface(
        setup->m_instance, setup->m_window, VK_NULL_HANDLE, &setup->m_surface);

#endif // _WIN32
    if (setup->result != VK_SUCCESS) {
        THROW("failed to create window surface!", setup->result);
    }
}
