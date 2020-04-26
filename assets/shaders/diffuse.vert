#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNorm;

out vec2 TexCoord;
out vec3 Normal;
out vec3 LightDirection;
out vec3 EyeDirection;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 light_pos;

void main() {
        TexCoord = aTexCoord;
        
        // Vertex's position in modelspace
        vec4 vert_position_model = vec4(aPos, 1.0);

        // Vertex's position in worldspace
        vec4 vert_position_world = model * vert_position_model;

        // Vertex's position in cameraspace
        vec4 vert_position_camera = view * vert_position_world;

        // Vertex's position in clipspace
        gl_Position = projection * vert_position_camera;

        // Light's position in cameraspace
        vec3 light_pos_camera = (view * vec4(light_pos, 1)).xyz;

        // Vector from vertex to camera in cameraspace
        vec3 eye_direction_camera = vec3(0,0,0) - vert_position_camera.xyz;
        EyeDirection = eye_direction_camera;

        // Vector from light to vertex in camera space
        LightDirection = light_pos_camera + eye_direction_camera;

        // Normal of the vertex in cameraspace
        mat3 invtrans_model = mat3(transpose(inverse(model)));
        Normal = mat3(view) * invtrans_model * aNorm;
}
