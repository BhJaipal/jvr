#version 450
// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
