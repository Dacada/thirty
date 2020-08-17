#ifndef OBJECT_H
#define OBJECT_H

#include <componentCollection.h>
#include <material.h>
#include <shader.h>
#include <dsutils.h>
#include <cglm/struct.h>

#define OBJECT_TREE_MAXIMUM_DEPTH 256

/*
 * This module offers an implementation of an object. Objects follow a tree
 * organization and each object has one parent (except the root object) and can
 * have several children. An object may have a geometry and a material
 * associated. An object with geometry but without a material will present
 * undefined behavior when trying to draw it and will probably crash.
 */

struct object {
        size_t idx;
        char *name;
        struct scene *scene;
        
        size_t parent;
        struct growingArray children;
        
        mat4s model;
        struct componentCollection components;
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
void object_initEmpty(struct object *object, struct scene *scene,
                      const char *name)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Initialize object from a file pointer. Does not set parent or children. This
 * is expected to be a BOGLE file that's already pointing at an object header,
 * and it will read all of the header and the data then return without doing
 * anything else to the file object.
 */
void object_initFromFile(struct object *object, struct scene *scene,
                         unsigned ncams, unsigned ngeos,
                         unsigned nmats, unsigned nlights,
                         unsigned nanims, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((access (read_write, 8)))
        __attribute__((nonnull));

void object_setSkybox(struct object *skybox, size_t geo, size_t mat)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

void object_translate(struct object *object, vec3s delta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void object_translateX(struct object *object, float delta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void object_translateY(struct object *object, float delta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void object_translateZ(struct object *object, float delta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void object_rotate(struct object *object, float angle, vec3s axis)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void object_rotateX(struct object *object, float angle)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void object_rotateY(struct object *object, float angle)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void object_rotateZ(struct object *object, float angle)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void object_rotateMat(struct object *object, mat4s rotation)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void object_scale(struct object *object, vec3s scale)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void object_addChild(struct object *parent, struct object *child)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));


void object_update(struct object *object)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

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

void object_free(struct object *object)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

#endif /* OBJECT_H */
