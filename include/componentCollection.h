#ifndef COMPONENT_COLLECTION_H
#define COMPONENT_COLLECTION_H

#include <animationCollection.h>
#include <camera.h>
#include <geometry.h>
#include <light.h>
#include <material.h>
#include <physicalEntity.h>
#include <transform.h>

/*
 * A collection of components assigned to an object. Each component exists
 * within a memory area managed by the componentCollection module, so they must
 * be created from it.
 */

struct componentCollection {
        size_t transform;
        size_t camera;
        size_t geometry;
        size_t material;
        size_t light;
        size_t animationCollection;
        size_t physicalEntity;
};

/*
 * Initialize the componentCollection module, allowing the creation and
 * management of object components.
 */
void componentCollection_startup(void);

/*
 * Allocate memory for a component of the given type and return it. Of course,
 * different types will have different sizes.
 */
void *componentCollection_create(struct game *game, enum componentType type)
        __attribute__((returns_nonnull));

/*
 * Obtain a component's idx by their name and optionally type. If type is
 * COMPONENT_TOTAL then all types are considered. If the component is not found
 * 0 is returned, otherwise idx+1 so 1 should be substracted from the result.
 */
size_t componentCollection_idxByName(const char *name,
                                     enum componentType type)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Return a pointer to a component from its idx. Note that this pointer may be
 * invalidated after calls to componentCollection_create, so it's better to
 * keep the idx around and access the component by idx when needed.
 */
void *componentCollection_compByIdx(size_t idx);

/*
 * Initialize a component collection.
 */
void componentCollection_init(struct componentCollection *collection)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Get the component of a given type for this collection. Or NULL if this
 * collection doesn't have a component in the slot for that type.
 */
void *componentCollection_get(
        const struct componentCollection *collection,
        enum componentType type)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Set the component for the given type slot to the given idx for the given
 * object.
 */
void componentCollection_set(struct componentCollection *collection,
                             size_t object,
                             enum componentType type,
                             size_t idx)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Return whether the given collection has a component in the given type slot.
 */
bool componentCollection_hasComponent(
        const struct componentCollection *collection,
        enum componentType type)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Update all components in the collection. To be called once per frame.
 */
void componentCollection_update(struct componentCollection *collection,
                                float timeDelta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Free any and all resources used by the componentCollection, deinitializing
 * it.
 */
void componentCollection_free(const struct componentCollection *collection)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Shut down the component collection system.
 */
void componentCollection_shutdown(void)
        __attribute__((nonnull));

#endif /* COMPONENT_COLLECTION_H */
