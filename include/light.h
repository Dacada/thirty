#ifndef LIGHT_H
#define LIGHT_H

#include <cglm/struct.h>

/*
 * TODO: Properly write descriptions when lightning is all properly structured
 * and defined.
 */

struct light {
        vec3s position;
        vec4s ambientColor;
        vec4s difuseColor;
        vec4s specularColor;
        float ambientPower;
        float difusePower;
        float specularPower;
};

/*
 * Update shader with light info. Probably only needs to be called once per
 * frame per shader.
 */
void light_update_shader(const struct light *restrict light, unsigned shader)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* LIGHT_H */