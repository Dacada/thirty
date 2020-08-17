#ifndef COMPONENT_H
#define COMPONENT_H

#include <stddef.h>

/*
 * Defines a component. Components are attached to objects through a
 * componentCollection. Components define all the characteristics of an object:
 * mesh, material, animation, light, camera... There are also subtypes of some
 * components.
 */

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

        COMPONENT_ANIMATIONCOLLECTION,
        
        COMPONENT_TOTAL
};

struct component {
        enum componentType type;
        size_t idx;
        char *name;
} __attribute__((aligned (COMPONENT_STRUCT_ALIGNMENT)));

/*
 * Initialize a component. The name can be freed after this call.
 */
void component_init(struct component *component, const char *name);

/*
 * Free any resources used by a component.
 */
void component_free(struct component *component);

#endif /* COMPONENT_H */
