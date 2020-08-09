#include <componentCollection.h>
#include <geometry.h>
#include <camera.h>
#include <material.h>
#include <light.h>
#include <texture.h>
#include <component.h>
#include <dsutils.h>
#include <util.h>
#include <stdbool.h>

#define COMPONENTS_MIN_SIZE sizeof(struct component)
#define COMPONENTS_INITIAL_CAPACITY (4*COMPONENTS_MIN_SIZE)

static struct varSizeGrowingArray components;

void componentCollection_startup(void) {
        varSizeGrowingArray_init(&components, COMPONENT_STRUCT_ALIGNMENT,
                                 COMPONENTS_INITIAL_CAPACITY,
                                 COMPONENTS_MIN_SIZE);
}

void *componentCollection_create(const enum componentType type) {
        size_t size;
        switch(type) {
        case COMPONENT_CAMERA_BASIC:
                size = sizeof(struct camera_basic);
                break;
                
        case COMPONENT_CAMERA_FPS:
                size = sizeof(struct camera_fps);
                break;
                
        case COMPONENT_GEOMETRY:
                size = sizeof(struct geometry);
                break;
                
        case COMPONENT_MATERIAL_UBER:
                size = sizeof(struct material_uber);
                break;
                
        case COMPONENT_MATERIAL_SKYBOX:
                size = sizeof(struct material_skybox);
                break;
                
        case COMPONENT_LIGHT_SPOT:
        case COMPONENT_LIGHT_DIRECTION:
        case COMPONENT_LIGHT_POINT:
                size = sizeof(struct light);
                break;
        
        case COMPONENT_TOTAL:
        default:
                dbg("comp: %u", type);
                assert_fail();
        }

        void *ptr = varSizeGrowingArray_append(&components, size);
        assert(components.offsets.length > 0);
        ((struct component*)ptr)->type = type;
        ((struct component*)ptr)->idx = components.offsets.length - 1;
        return ptr;
}

void componentCollection_init(struct componentCollection *const collection) {
        collection->camera = 0;
        collection->geometry = 0;
        collection->material = 0;
        collection->light = 0;
}

struct component *componentCollection_get(
        const struct componentCollection *const collection,
        const enum componentType type){

        size_t idx;
        switch (type) {
        case COMPONENT_CAMERA:
        case COMPONENT_CAMERA_FPS:
                idx = collection->camera;
                break;
                
        case COMPONENT_GEOMETRY:
                idx = collection->geometry;
                break;
                
        case COMPONENT_MATERIAL:
        case COMPONENT_MATERIAL_SKYBOX:
                idx = collection->material;
                break;
                
        case COMPONENT_LIGHT:
        case COMPONENT_LIGHT_DIRECTION:
        case COMPONENT_LIGHT_POINT:
                idx = collection->light;
                break;
                
        case COMPONENT_TOTAL:
        default:
                return NULL;
        }

        if (idx == 0) {
                return NULL;
        }

        idx--;

        return varSizeGrowingArray_get(&components, idx, NULL);
}

void componentCollection_set(struct componentCollection *const collection,
                             const enum componentType type,
                             const size_t idx) {
        switch (type) {
        case COMPONENT_CAMERA:
        case COMPONENT_CAMERA_FPS:
                collection->camera = idx+1;
                break;
                
        case COMPONENT_GEOMETRY:
                collection->geometry = idx+1;
                break;
                
        case COMPONENT_MATERIAL:
        case COMPONENT_MATERIAL_SKYBOX:
                collection->material = idx+1;
                break;
                
        case COMPONENT_LIGHT:
        case COMPONENT_LIGHT_DIRECTION:
        case COMPONENT_LIGHT_POINT:
                collection->light = idx+1;
                break;
                
        case COMPONENT_TOTAL:
        default:
                assert_fail();
                break;
        }
}

bool componentCollection_hasComponent(
        const struct componentCollection *const collection,
        const enum componentType type) {
        
        switch (type) {
        case COMPONENT_CAMERA:
        case COMPONENT_CAMERA_FPS:
                return collection->camera != 0;
                
        case COMPONENT_GEOMETRY:
                return collection->geometry != 0;
                
        case COMPONENT_MATERIAL:
        case COMPONENT_MATERIAL_SKYBOX:
                return collection->material != 0;
                
        case COMPONENT_LIGHT:
        case COMPONENT_LIGHT_DIRECTION:
        case COMPONENT_LIGHT_POINT:
                return collection->light != 0;
                
        case COMPONENT_TOTAL:
        default:
                return false;
        }
}

void componentCollection_free(
        const struct componentCollection *const collection) {
        (void)collection;
}

static bool freeComponent(void *compPtr, size_t size, void *args) {
        (void)args;
        (void)size;
        struct component *comp = compPtr;
        
        switch(comp->type) {
        case COMPONENT_CAMERA:
        case COMPONENT_CAMERA_FPS:
                camera_free((struct camera*)comp);
                break;
                
        case COMPONENT_GEOMETRY:
                geometry_free((struct geometry*)comp);
                break;
                
        case COMPONENT_MATERIAL:
        case COMPONENT_MATERIAL_SKYBOX:
                material_free((struct material*)comp);
                break;
                
        case COMPONENT_LIGHT:
        case COMPONENT_LIGHT_DIRECTION:
        case COMPONENT_LIGHT_POINT:
                light_free((struct light*)comp);
                break;
                
        case COMPONENT_TOTAL:
        default:
                break;
        }

        return true;
}

void componentCollection_shutdown(void) {
        varSizeGrowingArray_foreach(&components, freeComponent, NULL);
        varSizeGrowingArray_destroy(&components);
}
