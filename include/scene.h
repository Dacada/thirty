#ifndef SCENE_H
#define SCENE_H

#include <object.h>
#include <dsutils.h>
#include <cglm/struct.h>

/*
 * A scene contains a collection of objects (children of 'root'). One of these
 * objects is the camera. And some of them may be lights. The scene also
 * contains pointers to camera, light, material shader and object data which
 * are only held for easy freeing later.  Drawing a scene means drawing the
 * objects taking into account and their tree structure, using the light and
 * camera objects appropiately.
 */

struct scene {
        struct object root;
        struct growingArray objects;
        vec4s globalAmbientLight;
};

void scene_initFromFile(struct scene *scene, const char *filename)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

struct object *scene_createObject(struct scene *scene, size_t parent_idx)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

struct object *scene_getObjectFromIdx(struct scene *scene,
                                      size_t object_idx)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

size_t scene_setSkybox(struct scene *scene, const char *basename)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void scene_draw(const struct scene *scene)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void scene_free(struct scene *scene)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* SCENE_H */
