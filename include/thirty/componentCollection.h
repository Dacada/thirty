#ifndef COMPONENT_COLLECTION_H
#define COMPONENT_COLLECTION_H

#include <thirty/animationCollection.h>
#include <thirty/camera.h>
#include <thirty/geometry.h>
#include <thirty/light.h>
#include <thirty/material.h>
#include <thirty/transform.h>
#include <thirty/dsutils.h>

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
};

/*
 * Initialize the componentCollection memory area.
 */
void componentCollection_initCollection(struct varSizeGrowingArray *components);

/*
 * Allocate memory for a component of the given type and return it. Of course,
 * different types will have different sizes.
 */
void *componentCollection_create(struct varSizeGrowingArray *components,
                                 struct game *game, enum componentType type)
        __attribute__((returns_nonnull));

/*
 * Obtain a component's idx by their name and optionally type. If type is
 * COMPONENT_TOTAL then all types are considered. If the component is not found
 * 0 is returned, otherwise idx+1 so 1 should be substracted from the result.
 */
size_t componentCollection_idxByName(struct varSizeGrowingArray *components,
                                     const char *name, enum componentType type)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Return a pointer to a component from its idx. Note that this pointer may be
 * invalidated after calls to componentCollection_create, so it's better to
 * keep the idx around and access the component by idx when needed.
 */
void *componentCollection_compByIdx(struct varSizeGrowingArray *components, size_t idx);

/*
 * Initialize a component collection.
 */
void componentCollection_init(struct componentCollection *collection)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Function used when reading a scene from a file, return the current offset:
 * the number of components that exist in total.
 */
size_t componentCollection_currentOffset(struct varSizeGrowingArray *components);

/*
 * Get the component of a given type for this collection. Or NULL if this
 * collection doesn't have a component in the slot for that type.
 */
void *componentCollection_get(
        struct varSizeGrowingArray *components,
        const struct componentCollection *collection,
        enum componentType type)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Set the component for the given type slot to the given idx for the given
 * object.
 */
void componentCollection_set(
        struct varSizeGrowingArray *components,
        struct componentCollection *collection,
        size_t object, enum componentType type, size_t idx)
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
void componentCollection_update(
        struct varSizeGrowingArray *components,
        struct componentCollection *collection,
        float timeDelta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Free up the component collection memory area and all of its components.
 */
void componentCollection_freeCollection(struct varSizeGrowingArray *components)
        __attribute__((nonnull));

#endif /* COMPONENT_COLLECTION_H */
