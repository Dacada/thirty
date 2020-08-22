#include <componentCollection.h>
#include <texture.h>
#include <component.h>
#include <dsutils.h>
#include <util.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

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
        case COMPONENT_TRANSFORM:
                size = sizeof(struct transform);
                break;
                
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
                
        case COMPONENT_ANIMATIONCOLLECTION:
                size = sizeof(struct animationCollection);
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

struct findComponentArgs {
        const char *name;
        enum componentType type;
        size_t idx;
        bool found;
};
static bool findComponent(void *const item, const size_t size,
                          void *const vargs) {
        struct component *comp = item;
        struct findComponentArgs *args = vargs;
        (void)size;

        args->idx++;
        if ((args->type == COMPONENT_TOTAL || args->type == comp->type) &&
            strcmp(comp->name, args->name) == 0) {
                args->found = true;
                return false;
        }

        return true;
}
size_t componentCollection_idxByName(const char *const name,
                                     const enum componentType type) {
        struct findComponentArgs args = {
                .name = name,
                .type = type,
                .idx = 0,
                .found = false,
        };
        varSizeGrowingArray_foreach(&components, findComponent, &args);

        if (!args.found) {
                return 0;
        }
        return args.idx;
}

void *componentCollection_compByIdx(const size_t idx) {
        return varSizeGrowingArray_get(&components, idx, NULL);
}

void componentCollection_init(struct componentCollection *const collection) {
        collection->transform = 0;
        collection->camera = 0;
        collection->geometry = 0;
        collection->material = 0;
        collection->light = 0;
        collection->animationCollection = 0;
}

void *componentCollection_get(
        const struct componentCollection *const collection,
        const enum componentType type){

        size_t idx;
        switch (type) {
        case COMPONENT_TRANSFORM:
                idx = collection->transform;
                break;
                
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
                
        case COMPONENT_ANIMATIONCOLLECTION:
                idx = collection->animationCollection;
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
        case COMPONENT_TRANSFORM:
                collection->transform = idx+1;
                break;
                
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
                
        case COMPONENT_ANIMATIONCOLLECTION:
                collection->animationCollection = idx+1;
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
        case COMPONENT_TRANSFORM:
                return collection->transform != 0;
                
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
                
        case COMPONENT_ANIMATIONCOLLECTION:
                return collection->animationCollection != 0;
                
        case COMPONENT_TOTAL:
        default:
                return false;
        }
}

void componentCollection_update(struct componentCollection *const collection,
                                const float timeDelta) {
        struct animationCollection *anim = componentCollection_get(
                collection, COMPONENT_ANIMATIONCOLLECTION);
        if (anim != NULL) {
                animationCollection_update(anim, timeDelta);
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
        case COMPONENT_TRANSFORM:
                transform_free((struct transform*)comp);
                break;
                
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
                
        case COMPONENT_ANIMATIONCOLLECTION:
                animationCollection_free((struct animationCollection*)comp);
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
