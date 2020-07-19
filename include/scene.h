#ifndef SCENE_H
#define SCENE_H

#include <camera.h>
#include <light.h>
#include <material.h>
#include <object.h>

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
        
        vec4s globalAmbientLight;
        size_t nlights;
        struct light *lights;

        unsigned nmats;
        struct material *mats;
        
        unsigned nobjs;
        struct object *objs;

        unsigned ngeos;
        struct geometry *geos;

        struct camera *cam;
};

void scene_initFromFile(struct scene *scene, const char *filename)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void scene_draw(const struct scene *scene)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void scene_free(const struct scene *scene)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* SCENE_H */
