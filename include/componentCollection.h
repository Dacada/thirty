#ifndef COMPONENT_COLLECTION_H
#define COMPONENT_COLLECTION_H

#include <component.h>
#include <stdbool.h>

struct componentCollection {
        size_t camera;
        size_t geometry;
        size_t material;
        size_t light;
        size_t animationCollection;
};

void componentCollection_startup(void)
        __attribute__((nonnull));

void *componentCollection_create(enum componentType type);

size_t componentCollection_idxByName(const char *name,
                                     enum componentType type)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

void *componentCollection_compByIdx(size_t idx);

void componentCollection_init(struct componentCollection *collection)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

void *componentCollection_get(
        const struct componentCollection *collection,
        enum componentType type)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

void componentCollection_set(struct componentCollection *collection,
                             enum componentType type,
                             size_t idx)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

bool componentCollection_hasComponent(
        const struct componentCollection *collection,
        enum componentType type)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

void componentCollection_update(struct componentCollection *collection)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void componentCollection_free(const struct componentCollection *collection)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

void componentCollection_shutdown(void)
        __attribute__((nonnull));

#endif /* COMPONENT_COLLECTION_H */
