#ifndef LIGHT_H
#define LIGHT_H

#include <shader.h>
#include <component.h>
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

struct light {
        struct component base;
        bool enabled;

        float attenuation_constant;
        float attenuation_linear;
        float attenuation_quadratic;
        
        vec4s color;
        float intensity;
        float angle;
};


size_t light_initFromFile(struct light *light, FILE *f, enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Update shader with light info. Which is the index of the light in the shader.
 */
void light_updateShader(const struct light *light,
                        size_t which, mat4s view, mat4s model,
                        enum shaders shader)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Set all lights in the shader starting with the given which as disabled.
 */
void light_updateShaderDisabled(size_t which, enum shaders shader)
        __attribute__((leaf))
        __attribute__((nonnull));

/*
 * Update shader's global ambient lighting
 */
void light_updateGlobalAmbient(enum shaders shader, vec4s globalAmbientLight)
        __attribute__((leaf));

#define LIGHT_MAXIMUM_SIZE sizeof(struct light)

#endif /* LIGHT_H */
