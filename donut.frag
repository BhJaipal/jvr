#version 450
// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal
layout(location = 0) in float zDist;
layout(location = 0) out vec4 FragColor;

void main() {
	float brightness = clamp(zDist / 6.0 + 0.5, 0.0, 1.0);
	FragColor = vec4(vec3(brightness), 1);
}
