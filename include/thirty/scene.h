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
        
        vec4s globalAmbientLight;
        
        struct object root;
        struct growingArray objects;
        
        struct varSizeGrowingArray components;

        struct growingArray loadSteps;
        struct growingArray freePtrs;

        struct growingArray loadingStack;
        size_t totalSizeFinishedAsyncLoad;
        bool loading;
        bool loaded;
};

typedef bool(*scene_loadCallback)(struct scene*, void*);

struct scene_loadStep {
        scene_loadCallback cb;
        void *args;
};

/*
 * Initialize a scene from parameters
 */
void scene_init(struct scene *scene, struct game *game,
                vec4s globalAmbientLight, size_t initalObjectCapacity)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Initialize a scene from a BOGLE file.
 */
void scene_initFromFile(struct scene *scene, struct game *game, const char *filename)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Take a step in loading a scene. It will load everything defined either in
 * the BOGLE file or through the scene_addLoadingStep functions. It returns
 * false if the function needs to be called again later to continue
 * loading. And true if the loading process finished. No new scenes should be
 * created between the first call to this function and the function finally
 * returning true.
 */
bool scene_load(struct scene *scene)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Unload a scene, freeing all resources. The scene is NOT deinitialized and
 * can be loaded again with a call to scene_load.
 */
void scene_unload(struct scene *scene)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Return whether all of the scene's async loaders have finished.
 */
bool scene_awaitAsyncLoaders(struct scene *scene)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Add a function to be called during the scene loading process. These
 * functions shall be called sequentially in the same order they were added
 * when scene_load is called. The args pointer must be either NULL or a pointer
 * that can be passed to free().
 *
 * However, if this function is called while a scene is being loaded, then the
 * new step will be executed only once immediately after the current one and
 * the parameter will not be free'd.
 *
 * If the callback returns true then the next function will be executed
 * immediately. If it returns false then it won't be executed until the next
 * call to scene_load.
 */
void scene_addLoadingStep(struct scene *scene, scene_loadCallback cb, void *args)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull (1,2)));

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
 * Remove an object from the scene. The memory used from the object may be
 * freed later on or reused for other objects, but it should be considered
 * inaccessible immediately after a call to this function. A call to this
 * function does not invalidate pointers to any other objects. The object's
 * parent will take as children the object's children, if any. This function
 * may not be called on the root object. This MUST be called with an object
 * belonging to the given scene.
 */
void scene_removeObject(struct scene *scene, struct object *object)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

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
