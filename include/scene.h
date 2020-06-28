#ifndef SCENE_H
#define SCENE_H

#include <object.h>
#include <light.h>
#include <camera.h>

struct scene {
        struct object root;
        struct light lights[LIGHTLIMIT];
        struct camera camera;
        
        unsigned nobjs;
        struct object *objs;
};

/*
 * Must also pass the width and height of the screen for correct camera
 * initialization.
 */
unsigned scene_initFromFile(struct scene *restrict scene, 
                            float width, float height,
                            const char *restrict filename)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 4)))
        __attribute__((leaf))
        __attribute__((nonnull));

void scene_draw(const struct scene *restrict scene)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void scene_free(const struct scene *restrict scene)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* SCENE_H */
