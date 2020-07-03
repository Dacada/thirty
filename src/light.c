#include <light.h>
#include <shader.h>
#include <cglm/struct.h>
#include <assert.h>

#define BUFFER_SIZE 256

vec4s light_globalAmbientLight = {
        .x = 0.1F,
        .y = 0.1F,
        .z = 0.1F,
        .w = 1.0F,
};

void light_updateShader(const struct light *const light,
                        const size_t numlights,
                        const enum shaders shader) {
        
#define SET_SHADER(format, shader_set, val)                  \
        snprintf(buf, BUFFER_SIZE, format, i);               \
        shader_set(shader, buf, val);

        assert(numlights <= NUM_LIGHTS);
        
        shader_use(shader);
        
        char buf[BUFFER_SIZE];
        for (size_t i=0; i<numlights; i++) {
                SET_SHADER("lights[%lu].enabled", shader_setBool, light->enabled);
        
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
                                           shader_setVec4, light->position_ws);
                                SET_SHADER("lights[%lu].position_vs",
                                           shader_setVec4, light->position_vs);
                                if (light->type == LIGHTTYPE_POINT) {
                                        break;
                                }
                                __attribute__ ((fallthrough));
                        case LIGHTTYPE_DIRECTION:
                                SET_SHADER("lights[%lu].direction_ws",
                                           shader_setVec4, light->direction_ws);
                                SET_SHADER("lights[%lu].direction_ws",
                                           shader_setVec4, light->direction_vs);
                                break;
                        }
                }
        }

        for (size_t i=numlights; i<NUM_LIGHTS; i++) {
                SET_SHADER("lights[%lu].enabled", shader_setBool, false);
        }
        
#undef SET_SHADER

}

void light_updateGlobalAmbient(const enum shaders shader) {
        shader_use(shader);
        shader_setVec4(shader, light_globalAmbientLight);
}
