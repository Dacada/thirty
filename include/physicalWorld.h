#ifndef PHYSICAL_WORLD_H
#define PHYSICAL_WORLD_H

#include <dsutils.h>

struct physicalWorld {
        size_t idx;
        float accumulatedTime;
        struct growingArray entities;
};

void physicalWorld_startup(void);

struct physicalWorld *physicalWorld_create(void)
        __attribute__((returns_nonnull));

void physicalWorld_init(struct physicalWorld *world)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

void physicalWorld_addEntity(struct physicalWorld *world, size_t idx)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void physicalWorld_free(struct physicalWorld *world)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void physicalWorld_shutdown(void);

#endif
