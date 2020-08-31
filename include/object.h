#ifndef OBJECT_H
#define OBJECT_H

#include <componentCollection.h>
#include <material.h>
#include <shader.h>
#include <eventBroker.h>
#include <dsutils.h>
#include <cglm/struct.h>
#include <stddef.h>

#define OBJECT_TREE_MAXIMUM_DEPTH 256

/*
 * This module offers an implementation of an object. Objects follow a tree
 * organization and each object has one parent (except the root object) and can
 * have several children. An object has a componentCollection associated with
 * it. An object with geometry but without a material will present undefined
 * behavior when trying to draw it and will probably crash. The onUpdate method
 * will be fired with the scene's update event. It's automatically registered
 * and it's register parameter is the object struct itself.
 */

struct object {
        size_t idx;
        char *name;
        size_t scene;
        struct game *game;
        
        size_t parent;
        struct growingArray children;

        struct componentCollection components;
        
        eventBrokerCallback onUpdate;
};

enum renderStage {
        RENDER_OPAQUE_OBJECTS,
        RENDER_TRANSPARENT_OBJECTS,
        RENDER_SKYBOX,
};

/*
 * Initialize an empty object with default paramters. WARNING! This object has
 * no parent or children defined! Use object_addChild for that.
 */
void object_initEmpty(struct object *object, struct game *game, size_t scene,
                      const char *name)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Initialize object from a file pointer. Does not set parent or children. This
 * is expected to be a BOGLE file that's already pointing at an object header,
 * and it will read all of the header and the data.
 */
void object_initFromFile(struct object *object, struct game *game,
                         size_t scene,
                         unsigned ncams, unsigned ngeos,
                         unsigned nmats, unsigned nlights,
                         unsigned nanims, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((access (read_write, 9)))
        __attribute__((nonnull));

/*
 * Set parent-child relationship between two objects.
 */
void object_addChild(struct object *parent, struct object *child)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

/*
 * Assign a component to the object's component collection.
 */
void object_setComponent(struct object *object, struct component *comp)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

/*
 * Retrieve the component in the object's collection, or NULL if there's no
 * component in that slot.
 */
void *object_getComponent(const struct object *object,
                          enum componentType type)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Update all of object's components.
 */
void object_update(struct object *object, float timeDelta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Draw an object. Use the given model amtrix, which should be evaluated, and a
 * view and projection matrix. The rest of parameters are better explained in
 * scene_draw
 */
bool object_draw(const struct object *object, mat4s model,
                 mat4s view, mat4s projection,
                 enum renderStage *lastRenderStage,
                 const struct material **lastMaterial,
                 enum shaders *lastShader)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_write, 5)))
        __attribute__((access (read_write, 6)))
        __attribute__((access (read_write, 7)))
        __attribute__((nonnull));

/*
 * Free any resources used by the object, deinitializing it.
 */
void object_free(struct object *object)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

#endif /* OBJECT_H */
