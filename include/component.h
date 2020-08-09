#ifndef COMPONENT_H
#define COMPONENT_H

#include <stddef.h>

#define COMPONENT_STRUCT_ALIGNMENT 16

enum componentType {
        COMPONENT_CAMERA,
        COMPONENT_CAMERA_BASIC = COMPONENT_CAMERA,
        COMPONENT_CAMERA_FPS,
        
        COMPONENT_GEOMETRY,
        
        COMPONENT_MATERIAL,
        COMPONENT_MATERIAL_UBER = COMPONENT_MATERIAL,
        COMPONENT_MATERIAL_SKYBOX,
        
        COMPONENT_LIGHT,
        COMPONENT_LIGHT_SPOT = COMPONENT_LIGHT,
        COMPONENT_LIGHT_DIRECTION,
        COMPONENT_LIGHT_POINT,
        
        COMPONENT_TOTAL
};

struct component {
        enum componentType type;
        size_t idx;
        char *name;
} __attribute__((aligned (COMPONENT_STRUCT_ALIGNMENT)));

void component_init(struct component *component, const char *name);

void component_free(struct component *component);

#endif /* COMPONENT_H */