#ifndef SCENE_H
#define SCENE_H

#include <object.h>
#include <light.h>
#include <camera.h>

/*
 * A scene contains a collection of objects (children of 'root'), a camera to
 * be used to render them, and a collection of lights that act on them. Drawing
 * a scene means drawing the objects taking into account all of this, and their
 * tree structure.
 */

struct scene {
        struct object root;
        struct camera camera;
        vec4s globalAmbientLight;
        
        struct light *lights;
        size_t nlights;
        
        unsigned nobjs;
        struct object *objs;

        unsigned nmats;
        struct material *mats;

        unsigned nshaders;
        enum shaders *shaders;
};

/*
 * Must also pass the width and height of the screen for correct camera
 * initialization.
 */
unsigned scene_initFromFile(struct scene *scene, 
                            float width, float height,
                            const char *filename)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 4)))
        __attribute__((leaf))
        __attribute__((nonnull));

void scene_updateAllLighting(const struct scene *scene)
        __attribute__((access (read_only, 1)));

void scene_draw(const struct scene *scene)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void scene_free(const struct scene *scene)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* SCENE_H */
