#ifndef SCENE_H
#define SCENE_H

#include <thirty/object.h>

/*
 * A scene contains a collection of objects (all children of 'root'). The scene
 * also contains a growing array of the actual objects, and so is its owner and
 * the only one that can create new objects.
 */

struct scene {
        size_t idx;
        struct game *game;
        struct object root;
        struct growingArray objects;
        vec4s globalAmbientLight;
};

/*
 * Initialize a scene from parameters
 */
void scene_init(struct scene *scene, struct game *game,
                vec4s globalAmbientLight, size_t initalObjectCapacity)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Initialize a scene from a BOGLE file postioned at the right offset.
 */
void scene_initFromFile(struct scene *scene, struct game *game, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Create and return an object for the scene, returning a pointer to it. The
 * object is initialized but completely empty. A call to this function might
 * invalidate any returned object pointers so it's better to deal in terms of
 * the object's idx.
 */
struct object *scene_createObject(struct scene *scene, const char *name,
                                  size_t parent_idx)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Get an object's absolute transform by going up the scene tree until root. If
 * at any point an object doesn't have a transform component, returns the
 * identity matrix. Otherwise returnsthe result of multiplying the transforms
 * of every object up to the root.
 */
mat4s scene_getObjectAbsoluteTransform(struct scene *scene,
                                       const struct object *object)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Get an object's idx by the name.
 */
size_t scene_idxByName(const struct scene *scene, const char *name)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Get an object's pointer by its idx. This pointer might be invalidated by
 * calls to scene_createObject.
 */
struct object *scene_getObjectFromIdx(struct scene *scene,
                                      size_t object_idx)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));
const struct object *scene_getObjectFromIdxConst(const struct scene *scene,
                                                 size_t object_idx)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Set a skybox for the scene from the given base name. This name will be the
 * name of the object, geometry and material of the skybox as well as the
 * textures, which should be of the form basename_from.png, etc.
 */
size_t scene_setSkybox(struct scene *scene, const char *basename)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Update all objects in the scene, to be called once per frame.
 */
void scene_update(struct scene *scene, float timeDelta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Draw the scene using OpenGL.
 */
void scene_draw(const struct scene *scene)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Free all resources used by the scene, deinitializing it and all its objects.
 */
void scene_free(struct scene *scene)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

#endif /* SCENE_H */
