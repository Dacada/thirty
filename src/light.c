#include <light.h>
#include <util.h>
#include <shader.h>
#include <cglm/struct.h>
#include <assert.h>

#define BUFFER_SIZE 256

void light_initFromFile(struct light *const light,
                        FILE *const f) {
        light->enabled = true;
        
        sfread(light->color.raw, 4, 4, f);
        sfread(&light->range, 4, 1, f);
        sfread(&light->intensity, 4, 1, f);

        unsigned type;
        sfread(&type, 4, 1, f);
        light->type = type;

        sfread(light->position.raw, 4, 4, f);
        sfread(light->direction.raw, 4, 4, f);
        sfread(&light->angle, 4, 1, f);
}

#define SET_SHADER(format, shader_set, val)                  \
        snprintf(buf, BUFFER_SIZE, format, i);               \
        shader_set(shader, buf, val);

void light_updateShaderAll(const struct light *const lights,
                           const size_t nlights, const enum shaders shader) {
        assert(nlights <= NUM_LIGHTS);
        
        char buf[BUFFER_SIZE];
        for (size_t i=0; i<nlights; i++) {
                const struct light *const light = lights + i;
                
                SET_SHADER("lights[%lu].enabled", shader_setBool,
                           light->enabled);
        
                if (light->enabled) {
                        SET_SHADER("lights[%lu].color",
                                   shader_setVec4, light->color);
                        SET_SHADER("lights[%lu].range",
                                   shader_setFloat, light->range);
                        SET_SHADER("lights[%lu].intensity",
                                   shader_setFloat, light->intensity);
                        SET_SHADER("lights[%lu].type",
                                   shader_setUInt, light->type);
                        
                        switch (light->type) {
                        case LIGHTTYPE_SPOT:
                                SET_SHADER("lights[%lu].angle",
                                           shader_setFloat, light->angle);
                                __attribute__ ((fallthrough));
                        case LIGHTTYPE_POINT:
                                SET_SHADER("lights[%lu].position_ws",
                                           shader_setVec4, light->position);
                                if (light->type == LIGHTTYPE_POINT) {
                                        break;
                                }
                                __attribute__ ((fallthrough));
                        case LIGHTTYPE_DIRECTION:
                                SET_SHADER("lights[%lu].direction_ws",
                                           shader_setVec4, light->direction);
                                break;
                        default:
                                assert(false);
                                break;
                        }
                }
        }

        for (size_t i=nlights; i<NUM_LIGHTS; i++) {
                SET_SHADER("lights[%lu].enabled", shader_setBool, false);
        }

}

void light_updateShaderView(const struct light *const lights,
                            const size_t nlights, const mat4s viewMatrix,
                            const enum shaders shader) {
        assert(nlights <= NUM_LIGHTS);
        
        char buf[BUFFER_SIZE];
        for (size_t i=0; i<nlights; i++) {
                const struct light *const light = lights + i;
                
                if (light->enabled) {
                        switch (light->type) {
                        case LIGHTTYPE_SPOT:
                        case LIGHTTYPE_POINT:
                                ;
                                vec4s position_vs = glms_mat4_mulv(
                                        viewMatrix, light->position);
                                SET_SHADER("lights[%lu].position_vs",
                                           shader_setVec4, position_vs);
                                if (light->type == LIGHTTYPE_POINT) {
                                        break;
                                }
                                __attribute__ ((fallthrough));
                        case LIGHTTYPE_DIRECTION:
                                ;
                                vec4s direction_vs = glms_mat4_mulv(
                                        viewMatrix, light->direction);
                                SET_SHADER("lights[%lu].direction_vs",
                                           shader_setVec4, direction_vs);
                                break;
                        default:
                                assert(false);
                                break;
                        }
                }
        }
}
        
#undef SET_SHADER

void light_updateGlobalAmbient(const enum shaders shader,
                               const vec4s globalAmbientLight) {
        shader_setVec4(shader, "material.globalAmbient", globalAmbientLight);
}
