#include <thirty/componentCollection.h>
#include <thirty/util.h>

#define COMPONENTS_MIN_SIZE sizeof(struct component)
#define COMPONENTS_INITIAL_CAPACITY (4*COMPONENTS_MIN_SIZE)

void componentCollection_initCollection(struct varSizeGrowingArray *components) {
        varSizeGrowingArray_init(components, COMPONENT_STRUCT_ALIGNMENT,
                                 COMPONENTS_INITIAL_CAPACITY,
                                 COMPONENTS_MIN_SIZE);
}

void *componentCollection_create(struct varSizeGrowingArray *components, struct game *game,
                                 const enum componentType type) {
        size_t size;
        switch(type) {
        case COMPONENT_TRANSFORM:
                size = sizeof(struct transform);
                break;
                
        case COMPONENT_CAMERA:
                size = sizeof(struct camera);
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

        void *ptr = varSizeGrowingArray_append(components, size);
        assert(components->offsets.length > 0);
        ((struct component*)ptr)->type = type;
        ((struct component*)ptr)->idx = components->offsets.length - 1;
        ((struct component*)ptr)->game = game;
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
size_t componentCollection_idxByName(struct varSizeGrowingArray *components,
                                     const char *const name,
                                     const enum componentType type) {
        struct findComponentArgs args = {
                .name = name,
                .type = type,
                .idx = 0,
                .found = false,
        };
        varSizeGrowingArray_foreach(components, findComponent, &args);

        if (!args.found) {
                return 0;
        }
        return args.idx;
}

void *componentCollection_compByIdx(struct varSizeGrowingArray *components,
                                    const size_t idx) {
        return varSizeGrowingArray_get(components, idx, NULL);
}

void componentCollection_init(struct componentCollection *const collection) {
        collection->transform = 0;
        collection->camera = 0;
        collection->geometry = 0;
        collection->material = 0;
        collection->light = 0;
        collection->animationCollection = 0;
}

size_t componentCollection_currentOffset(struct varSizeGrowingArray *components) {
        return components->offsets.length;
}

void *componentCollection_get(
        struct varSizeGrowingArray *components,
        const struct componentCollection *const collection,
        const enum componentType type){

        size_t idx;
        switch (type) {
        case COMPONENT_TRANSFORM:
                idx = collection->transform;
                break;
                
        case COMPONENT_CAMERA:
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

        return varSizeGrowingArray_get(components, idx, NULL);
}

void componentCollection_set(
        struct varSizeGrowingArray *components,
        struct componentCollection *const collection,
        const size_t object, const enum componentType type, const size_t idx) {
        switch (type) {
        case COMPONENT_TRANSFORM:
                collection->transform = idx+1;
                break;
                
        case COMPONENT_CAMERA:
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

        struct component *component = varSizeGrowingArray_get(components, idx, NULL);
        component->object = object;
}

bool componentCollection_hasComponent(
        const struct componentCollection *const collection,
        const enum componentType type) {
        
        switch (type) {
        case COMPONENT_TRANSFORM:
                return collection->transform != 0;
                
        case COMPONENT_CAMERA:
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

void componentCollection_update(struct varSizeGrowingArray *components,
                                struct componentCollection *const collection,
                                const float timeDelta) {
        struct animationCollection *anim = componentCollection_get(components,
                collection, COMPONENT_ANIMATIONCOLLECTION);
        if (anim != NULL) {
                animationCollection_update(anim, timeDelta);
        }
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

void componentCollection_freeCollection(struct varSizeGrowingArray *components) {
        varSizeGrowingArray_foreach(components, freeComponent, NULL);
        varSizeGrowingArray_destroy(components);
}
