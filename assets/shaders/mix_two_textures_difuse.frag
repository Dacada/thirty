#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 LightDirection;
in vec3 EyeDirection;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec4 ambient_light_color;
uniform vec4 difuse_light_color;
uniform vec4 specular_light_color;
uniform float ambient_light_power;
uniform float difuse_light_power;
uniform float specular_light_power;

void main() {
        vec4 texture_color = mix(texture(texture0, TexCoord), texture(texture1, TexCoord), 0.2);
        vec4 ambient_texture_color = texture_color;
        vec4 difuse_texture_color = texture_color;
        vec4 specular_texture_color = texture_color;
        
        vec3 n = normalize(Normal);
        vec3 l = normalize(LightDirection);
        float light_distance = length(LightDirection);
        float cosTheta = clamp(dot(n, l), 0, 1);
        vec3 E = normalize(EyeDirection);
        vec3 R = reflect(-l, n);
        float cosAlpha = clamp(dot(E, R), 0, 1);
        
        vec4 ambient = ambient_texture_color * ambient_light_color * vec4(vec3(ambient_light_power), 1);
        
        vec4 difuse = difuse_texture_color * difuse_light_color * difuse_light_power * cosTheta / (light_distance * light_distance);

        vec4 specular = specular_texture_color * specular_light_color * specular_light_power * pow(cosAlpha, 5) / (light_distance * light_distance);
        
        FragColor = ambient + difuse + specular;
}
