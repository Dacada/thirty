#ifndef OBJECT_H
#define OBJECT_H

#define OBJECT_NAME_SIZE 32

#include <geometry.h>
#include <camera.h>
#include <light.h>
#include <shader.h>

/*
 * This module offers an implementation of an object. Objects follow a tree
 * organization and each object has one parent (except the root object) and can
 * have several children.
 */

struct object {
        struct object *parent;
        unsigned nchildren;
        struct object **children;

        char name[OBJECT_NAME_SIZE];
        mat4s model;

        struct geometry *geometry;
        struct material *material;
};

/*
 * Initialize object from a file pointer. Does not set parent or children. This
 * is expected to be a BOGLE file that's already pointing at an object header,
 * and it will read all of the header and the data then return without doing
 * anything else to the file object.
 */
void object_init_fromFile(struct object *restrict object, FILE *restrict f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_translate(struct object *restrict object, vec3s position)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_rotate(struct object *restrict object, float angle, vec3s axis)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_scale(struct object *restrict object, vec3s scale)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void object_draw(const struct object *restrict object,
                 const struct camera *restrict camera,
                 size_t nlights, size_t nshaders,
                 const struct light *lights,\
                 const enum shaders *shaders)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((access (read_only, 5, 3)))
        __attribute__((access (read_only, 6, 4)))
        __attribute__((leaf))
        __attribute__((nonnull (1, 2)));

void object_free(const struct object *restrict object)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* OBJECT_H */
