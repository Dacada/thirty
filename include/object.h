#ifndef OBJECT_H
#define OBJECT_H

#define OBJECT_NAME_SIZE 32

#include <geometry.h>
#include <camera.h>
#include <light.h>

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
        unsigned shader;
};

/*
 * Initialize object from a file pointer. Does not set parent or children. This
 * is expected to be a BOGLE file that's already pointing at an object header,
 * and it will read all of the header and the data then return without doing
 * anything else to the file object.
 */
void object_init_fromFile(struct object *restrict object, FILE *restrict f);

void object_translate(struct object *restrict object, vec3s position);
void object_rotate(struct object *restrict object, float angle, vec3s axis);
void object_scale(struct object *restrict object, vec3s scale);

void object_draw(const struct object *restrict object,
                 const struct camera *restrict camera,
                 const struct light lights[LIGHTLIMIT]);

void object_free(struct object *restrict object);

#endif /* OBJECT_H */
