#ifndef LIGHT_H
#define LIGHT_H

#include <shader.h>
#include <cglm/struct.h>

#define NUM_LIGHTS 20

/*
 * Contains definitions for lighting. For now this is forward shading wth 20
 * lights max. Three types of light supported: spot, direction and point. This
 * also keeps the global ambient light, a function to update a shader's
 * lighting data and a function to update the global ambient lighting value in
 * the shader (for when it's changed).
 */

extern vec4s light_globalAmbientLight;

enum lightType {
        LIGHTTYPE_SPOT = 0,
        LIGHTTYPE_DIRECTION = 1,
        LIGHTTYPE_POINT = 2
};

struct light {
        bool enabled;
        vec4s color;
        float range;
        float intensity;
        enum lightType type;
        
        // Only point and spot lights
        vec4s position;

        // Only spot and directional lights
        vec4s direction;

        // Only spot light
        float angle;
};


void light_initFromFile(struct light *light, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Update shader with all light info.
 */
void light_updateShaderAll(const struct light *lights,
                           size_t nlights, enum shaders shader)
        __attribute__((access (read_only, 1, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Update shader with viewspace information for lights.
 */
void light_updateShaderView(const struct light *lights,
                            size_t nlights, mat4s viewMatrix,
                            enum shaders shader)
        __attribute__((access (read_only, 1, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Update shader's global ambient lighting
 */
void light_updateGlobalAmbient(enum shaders shader, vec4s globalAmbientLight)
        __attribute__((leaf));

#endif /* LIGHT_H */
