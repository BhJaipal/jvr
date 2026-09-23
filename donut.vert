#version 450
// SPDX-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 BhJaipal

layout(push_constant) uniform UBO {
    mat4 mvp;
	vec2 rotate; //  x = A and y = B
} ubo;

layout(location = 0) in vec2 vertex; // x = theta, y = phi
layout(location = 0) out float zDist;

void main(){
	mat3 Rx = mat3(
		vec3(1, 0, 0),
		vec3(0,  cos(ubo.rotate.x), sin(ubo.rotate.x)),
		vec3(0, -sin(ubo.rotate.x), cos(ubo.rotate.x))
	);

	mat3 Rz = mat3(
		vec3( cos(ubo.rotate.y), sin(ubo.rotate.y), 0),
		vec3(-sin(ubo.rotate.y), cos(ubo.rotate.y), 0),
		vec3(0, 0, 1)
	);

	mat3 Ry = mat3(
		vec3( cos(vertex.y), 0, sin(vertex.y)),
		vec3(       0,       1,       0      ),
		vec3(-sin(vertex.y), 0, cos(vertex.y))
	);
	vec3 circle = vec3(2 + cos(vertex.x), sin(vertex.x), 0);
	vec3 donut = Rz * Rx * Ry * circle;
	gl_Position = ubo.mvp * vec4(donut, 1);
	zDist = donut.z;
}
