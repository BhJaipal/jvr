#version 450
// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal

layout(push_constant) uniform Obj {
    mat4 mvp;
} ubo;
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 fragColor;

void main() {
	gl_Position = ubo.mvp * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}

