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
unsigned scene_initFromFile(struct scene *scene, 
                            float width, float height,
                            const char *filename);

void scene_draw(const struct scene *scene);

void scene_free(const struct scene *scene);

#endif /* SCENE_H */
