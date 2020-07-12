#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texCoord;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec3 in_binormal;

out vec2 texCoord;
out vec3 position_vs;
out vec3 tangent_vs;
out vec3 binormal_vs;
out vec3 normal_vs;

uniform mat4 modelViewProjection;
uniform mat4 modelView;

void main() {
        gl_Position = modelViewProjection * vec4(in_position, 1);
        
        texCoord = in_texCoord;
        position_vs = (modelView * vec4(in_position, 1)).xyz;
        tangent_vs = mat3(modelView) * in_tangent;
        binormal_vs = mat3(modelView) * in_binormal;
        normal_vs = mat3(modelView) * in_normal;
}
