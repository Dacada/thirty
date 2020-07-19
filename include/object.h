#ifndef OBJECT_H
#define OBJECT_H

#include <camera.h>
#include <light.h>
#include <geometry.h>
#include <material.h>

#define OBJECT_TREE_MAXIMUM_DEPTH 256

/*
 * This module offers an implementation of an object. Objects follow a tree
 * organization and each object has one parent (except the root object) and can
 * have several children. An object may have a geometry and a material
 * associated. An object with geometry but without a material will present
 * undefined behavior when trying to draw it and will probably crash.
 */

struct object {
        struct object *parent;
        
        unsigned nchildren;
        struct object **children;

        mat4s model;

        struct camera *camera;
        struct geometry *geometry;
        struct material *material;
        struct light *light;
};

/*
 * Initialize object from a file pointer. Does not set parent or children. This
 * is expected to be a BOGLE file that's already pointing at an object header,
 * and it will read all of the header and the data then return without doing
 * anything else to the file object.
 */
void object_initFromFile(struct object *object,
                         struct geometry *geometries,
                         struct material *materials,
                         struct light *lights,
                         struct camera *camera, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_translate(struct object *object, vec3s delta)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_translateX(struct object *object, float delta)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_translateY(struct object *object, float delta)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_translateZ(struct object *object, float delta)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_rotate(struct object *object, float angle, vec3s axis)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_rotateX(struct object *object, float angle)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_rotateY(struct object *object, float angle)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_rotateZ(struct object *object, float angle)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_scale(struct object *object, vec3s scale)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_draw(const struct object *object)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_free(const struct object *object)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* OBJECT_H */
