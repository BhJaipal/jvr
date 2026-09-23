// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "jvr/render.hpp"
#include <cstring>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_enums.hpp>

int main(int argc, char *argv[])
{
    jvr::renderer render;

    const int LEN = 100 * 314 * 8;
    float *donut = new float[LEN];

    auto model = glm::mat4(1.0f);
    auto view = glm::lookAt(glm::vec3(0.0f, 0.f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.f, 0.0f));
    auto proj = glm::perspective(glm::radians(45.0f), 4.f / 3.f, 0.1f, 20.0f);
    proj[1][1] *= -1;
    glm::mat4 mvp = proj * view * model;

    auto config = new jvr::config;

    config->rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    config->shape(vk::PrimitiveTopology::eTriangleStrip);
    config->vertex_input_props(1, 1);
    config->vert_binding(0, 0, sizeof(glm::vec2), vk::VertexInputRate::eVertex);
    config->vert_attribute(0, 0, 0, vk::Format::eR32G32Sfloat, 0);
    config->add_push_constant(0, sizeof(glm::mat4) + sizeof(glm::vec2),
        vk::ShaderStageFlagBits::eVertex);
    config->bgClearColor({ 0.00625, 0.00625, 0.00625, 1.f });
    config->frame_sleep *= 2;

    try {
        render.validate();
        render.init(config, "Vulkan window");
        render.createGraphicsPipeline(
            "build/donut.vert.spv", "build/donut.frag.spv");
        render.createVertexBuffer(sizeof(float[LEN]));

        float A = 0;
        float B = 0;
        render.render([&A, &B, donut, mvp, LEN](jvr::renderer &render) {
            memset(donut, 0, LEN * 4);

            int donut_i = 0;
            float phi_diff = 0.02;
            float theta_diff = 0.0628;

            for (short i = 0; i < 314; i++) {
                float phi = phi_diff * i;
                float dphi = phi + phi_diff;
                if (i == 313)
                    dphi = 0;

                for (short j = 0; j < 100; j++) {
                    float theta = theta_diff * j;
                    float dtheta = theta + theta_diff;
                    if (j == 99)
                        dtheta = 0;

                    donut[donut_i++] = theta;
                    donut[donut_i++] = phi;

                    donut[donut_i++] = theta;
                    donut[donut_i++] = dphi;

                    donut[donut_i++] = dtheta;
                    donut[donut_i++] = phi;

                    donut[donut_i++] = dtheta;
                    donut[donut_i++] = dphi;
                }
            }
            glm::vec2 rotate = { A, B };
            render.push_constant(vk::ShaderStageFlagBits::eVertex, &mvp, //
                sizeof(mvp), 0);
            render.push_constant(vk::ShaderStageFlagBits::eVertex, &rotate,
                sizeof(rotate), sizeof(mvp));

            render.mapMemory(
                0, sizeof(float[LEN]), vk::MemoryMapFlagBits(), donut);

            render.bindVertexBuffers(0, 1);

            render.draw(LEN / 2, 1);

            A += 0.0704;
            B += 0.0352;
        });
    } catch (std::runtime_error err) {
        std::cout << err.what();
        if (render.check_result() != vk::Result::eSuccess)
            std::cout << ": " << vk::to_string(render.check_result());
        std::cout << "\n";
    }
    delete[] donut;
    render.destroy();
    return 0;
}
