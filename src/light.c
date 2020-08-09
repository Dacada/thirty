#include <light.h>
#include <shader.h>
#include <component.h>
#include <util.h>
#include <cglm/struct.h>

#define BUFFER_SIZE 256

size_t light_initFromFile(struct light *const light,
                          FILE *const f, const enum componentType type) {
        light->base.type = type;
        light->enabled = true;

        char *name = strfile(f);
        component_init((struct component*)light, name);
        free(name);
        
        sfread(light->color.raw, sizeof(float), 4, f);

        sfread(&light->attenuation_constant, sizeof(float), 1, f);
        sfread(&light->attenuation_linear, sizeof(float), 1, f);
        sfread(&light->attenuation_quadratic, sizeof(float), 1, f);
        
        sfread(&light->intensity, sizeof(float), 1, f);
        sfread(&light->angle, sizeof(float), 1, f);
        
        return sizeof(struct light);
}

#define SET_SHADER(format, shader_set, val)                  \
        snprintf(buf, BUFFER_SIZE, format, i);               \
        shader_set(shader, buf, val);

void light_updateShader(const struct light *light,
                        size_t which, mat4s view, mat4s model,
                        enum shaders shader) {
        char buf[BUFFER_SIZE];
        assert(which < NUM_LIGHTS);
        size_t i=which;

        SET_SHADER("lights[%lu].enabled", shader_setBool,
                   light->enabled);
        
        if (light->enabled) {
                SET_SHADER("lights[%lu].color",
                           shader_setVec4, light->color);

                SET_SHADER("lights[%lu].attenuation_constant",
                           shader_setFloat, light->attenuation_constant);
                SET_SHADER("lights[%lu].attenuation_linear",
                           shader_setFloat, light->attenuation_linear);
                SET_SHADER("lights[%lu].attenuation_quadratic",
                           shader_setFloat, light->attenuation_quadratic);
                
                SET_SHADER("lights[%lu].intensity",
                           shader_setFloat, light->intensity);
                SET_SHADER("lights[%lu].type",
                           shader_setUInt, light->base.type - COMPONENT_LIGHT);

                vec4s position;
                mat4s r;
                vec3s s;
                glms_decompose(model, &position, &r, &s);

                vec4s direction = {
                        .x=0, .y=0, .z=-1, .w=0
                };
                direction = glms_mat4_mulv(r, direction);
                
                vec4s position_vs = glms_mat4_mulv(view, position);
                vec4s direction_vs = glms_mat4_mulv(view, direction);
                        
                switch (light->base.type) {
                case COMPONENT_LIGHT_SPOT:
                        SET_SHADER("lights[%lu].angle",
                                   shader_setFloat, light->angle);
                        __attribute__ ((fallthrough));
                case COMPONENT_LIGHT_POINT:
                        SET_SHADER("lights[%lu].position_ws",
                                   shader_setVec4, position);
                        SET_SHADER("lights[%lu].position_vs",
                                   shader_setVec4, position_vs);
                        if (light->base.type == COMPONENT_LIGHT_POINT) {
                                break;
                        }
                        __attribute__ ((fallthrough));
                case COMPONENT_LIGHT_DIRECTION:
                        SET_SHADER("lights[%lu].direction_ws",
                                   shader_setVec4, direction);
                        SET_SHADER("lights[%lu].direction_vs",
                                   shader_setVec4, direction_vs);
                        break;

                case COMPONENT_CAMERA_BASIC:
                case COMPONENT_CAMERA_FPS:
                case COMPONENT_GEOMETRY:
                case COMPONENT_MATERIAL_UBER:
                case COMPONENT_MATERIAL_SKYBOX:
                case COMPONENT_TOTAL:
                default:
                        assert_fail();
                        break;
                }
        }
}

void light_updateShaderDisabled(size_t which, enum shaders shader) {
        char buf[BUFFER_SIZE];
        for (size_t i=which; i<NUM_LIGHTS; i++) {
                SET_SHADER("lights[%lu].enabled", shader_setBool, false);
        }
}

#undef SET_SHADER

void light_updateGlobalAmbient(const enum shaders shader,
                               const vec4s globalAmbientLight) {
        shader_setVec4(shader, "material.globalAmbient", globalAmbientLight);
}

void light_free(struct light *const light) {
        component_free((struct component*)light);
}
