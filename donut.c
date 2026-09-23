// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
#include "jvrc/render.h"
#include <stdlib.h>
#include <vulkan/vk_enum_string_helper.h>
#include "jvrc/inline.h"

const int LEN = 100 * 314 * 8;

float A = 0;
float B = 0;
float *donut;

void draw_donut(jvr_renderer *render);

int main(int argc, char *argv[])
{
    jvr_renderer *render = malloc(sizeof(jvr_renderer));

    donut = malloc(4 * LEN);

    jvr_config *config = calloc(1, sizeof(jvr_config));

    jvr_cfg_new(config);
    config->rasterizer.cullMode = VK_CULL_MODE_NONE;
    jvr_cfg_shape(config, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
    jvr_cfg_vertex_input_props(config, 1, 1);
    jvr_cfg_vert_binding(
        config, 0, 0, sizeof(float[2]), VK_VERTEX_INPUT_RATE_VERTEX);
    jvr_cfg_vert_attribute(config, 0, 0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
    jvr_cfg_add_push_constant(config, 0, sizeof(float[4][4]) + sizeof(float[2]),
        VK_SHADER_STAGE_VERTEX_BIT);
    jvr_cfg_bgClearColor(
        config, (VkClearColorValue){ 0.00625, 0.00625, 0.00625, 1.f });
    config->frame_sleep *= 2;

    jvr_render_validate(render);
    jvr_render_init(render, config, "Vulkan window", 800, 600);
    jvr_render_createGraphicsPipeline(
        render, "build/donut.vert.spv", "build/donut.frag.spv");
    jvr_createVertexBuffer(render, sizeof(float[LEN]));

    jvr_renderer_render(render, draw_donut);

    if (jvr_check_result(render) != VK_SUCCESS)
        printf("%s\n", string_VkResult(jvr_check_result(render)));
    free(donut);
    jvr_render_destroy(render);
    free(render);
    return 0;
}

void draw_donut(jvr_renderer *render)
{
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

    float mvp[4][4] = { { 1.810660, 0.000000, 0.000000, 0.000000 },
        { 0.000000, -2.414214, 0.000000, 0.000000 },
        { 0.000000, 0.000000, -1.010050, -1.000000 },
        { 0.000000, 0.000000, 9.899497, 10.000000 } };
    float rotate[2] = { A, B };
    jvr_push_constant(render, VK_SHADER_STAGE_VERTEX_BIT, mvp, //
        sizeof(mvp), 0);
    jvr_push_constant(render, VK_SHADER_STAGE_VERTEX_BIT, rotate,
        sizeof(rotate), sizeof(mvp));

    jvr_map_memory(render, 0, sizeof(float[LEN]), 0, donut);

    jvr_bindVertexBuffers(render, 0, 1);

    jvr_render_draw(render, LEN / 2, 1, 0, 0);

    A += 0.0704;
    B += 0.0352;
}
