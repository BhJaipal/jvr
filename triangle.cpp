// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include <cmath>
#include <cstddef>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "jvr/render.hpp"

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};

int main(int argc, char *argv[])
{
    jvr::renderer render;
    render.validate();

    auto model = glm::mat4(1.0f);
    auto view = glm::lookAt(glm::vec3(0.0f, 2.5f, 1.0f),
        glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.5f, 1.0f));
    auto proj = glm::perspective(glm::radians(45.0f), 4.f / 3.f, 0.1f, 10.0f);
    glm::mat4 mvp = proj * view * model;

    auto config = new jvr::config;
    config->depthStencil.depthTestEnable = VK_FALSE;
    config->vertex_input_props(1, 2);
    config->vert_binding(0, 0, sizeof(Vertex), vk::VertexInputRate::eVertex);
    config->vert_attribute(
        0, 0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos));
    config->vert_attribute(
        1, 0, 1, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));
    config->add_push_constant(
        0, sizeof(glm::mat4), vk::ShaderStageFlagBits::eVertex);

    float theta = 0;
    jvr::array<Vertex> vertices = { //
        { { cos(theta), sin(theta) }, { 1.0f, 0.2f, 0.2f } },
        { { cos(theta + 3.14 * 0.6667), sin(theta + 3.14 * 0.6667) },
            { 0.2f, 1.0f, 0.2f } },
        { { cos(theta + 3.14 * 2 * 0.6667), sin(theta + 3.14 * 2 * 0.6667) },
            { 0.2f, 0.2f, 1.0f } }
    };
    try {
        render.init(config, "Vulkan window");
        render.createGraphicsPipeline(
            "build/triangle.vert.spv", "build/triangle.frag.spv");
        render.createVertexBuffer(sizeof(Vertex) * 3);
        render.render([&theta, &vertices, mvp](jvr::renderer &render) {
            render.push_constant(
                vk::ShaderStageFlagBits::eVertex, &mvp, sizeof(mvp), 0);

            render.mapMemory(
                0, sizeof(Vertex) * 3, vk::MemoryMapFlagBits(), vertices.data);
            render.bindVertexBuffers(0, 1);
            render.draw(3, 1);

            theta += 0.0157;
            vertices[0].pos = glm::vec2(cos(theta), sin(theta));
            vertices[1].pos = glm::vec2(
                cos(theta + 3.14 * 0.6667), sin(theta + 3.14 * 0.6667));
            vertices[2].pos = glm::vec2(
                cos(theta + 3.14 * 2 * 0.6667), sin(theta + 3.14 * 2 * 0.6667));
        });
        render.destroy();
    } catch (std::runtime_error err) {
        std::cout << err.what();
        if (!render.check_result())
            std::cout << ": " << vk::to_string(render.check_result());
        std::cout << "\n";
    }
    return 0;
}
