#ifndef LIGHT_H
#define LIGHT_H

#include <shader.h>
#include <component.h>
#include <cglm/struct.h>
#include <stddef.h>

#define NUM_LIGHTS 20

/*
 * Light component definition. While there are many types of lights, in the end
 * they all use the same struct and so are the same size.
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

/*
 * Initialize a light from parameters. Attenuation is constant, linear,
 * quadratic.
 */
void light_init(struct light *light, enum componentType type, const char *name,
                vec3s attenuation, vec4s color, float intensity, float angle)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Initialize a light from a BOGLE file positioned at the correct offset.
 */
size_t light_initFromFile(struct light *light, FILE *f, enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

/*
 * Update shader with light info. Which is the index of the light in the shader.
 */
void light_updateShader(const struct light *light,
                        size_t which, mat4s view, mat4s model,
                        enum shaders shader)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Set all lights in the shader starting with the given which as disabled.
 */
void light_updateShaderDisabled(size_t which, enum shaders shader)
        __attribute__((nonnull));

/*
 * Update shader's global ambient lighting
 */
void light_updateGlobalAmbient(enum shaders shader, vec4s globalAmbientLight);

/*
 * Free all resources used by the light, deinitializing it.
 */
void light_free(struct light *light)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#define LIGHT_MAXIMUM_SIZE sizeof(struct light)

#endif /* LIGHT_H */
