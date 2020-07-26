#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texCoord;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec3 in_binormal;

uniform mat4 modelViewProjection;
uniform mat4 modelView;
