out vec2 texCoord;
out vec3 position_vs;
out vec3 tangent_vs;
out vec3 binormal_vs;
out vec3 normal_vs;
out mat4 invView;

void main() {
        gl_Position = modelViewProjection * vec4(in_position, 1);
        
        texCoord = in_texCoord;
        position_vs = (modelView * vec4(in_position, 1)).xyz;
        tangent_vs = mat3(modelView) * in_tangent;
        binormal_vs = mat3(modelView) * in_binormal;
        normal_vs = mat3(modelView) * in_normal;
        invView = inverse(view);
}
