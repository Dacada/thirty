#include <light.h>
#include <shader.h>

void light_update_shader(const struct light *const light,
                         const unsigned shader) {
        shader_use(shader);
        shader_setVec3(shader, "light_pos", light->position);
        shader_setVec4(shader, "ambient_light_color", light->ambientColor);
        shader_setVec4(shader, "difuse_light_color", light->difuseColor);
        shader_setVec4(shader, "specular_light_color", light->specularColor);
        shader_setFloat(shader, "ambient_light_power", light->ambientPower);
        shader_setFloat(shader, "difuse_light_power", light->difusePower);
        shader_setFloat(shader, "specular_light_power", light->specularPower);
}
