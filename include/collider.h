#ifndef COLLIDER_H
#define COLLIDER_H

#include <cglm/struct.h>

enum colliderType {
        COLLIDER_NONE,
        COLLIDER_SPHERE,
        COLLIDER_PLANE,
        COLLIDER_AABB,
};

struct collider_sphere {
        vec3s center;
        float radius;
};

struct collider_plane {
        vec3s normal;
        float distance;
};

struct collider_aabb {
        vec3s center;
        vec3s halfDistances;
};

struct collider {
        enum colliderType type;
        union {
                struct collider_sphere sphere;
                struct collider_plane plane;
                struct collider_aabb aabb;
        };
};

struct collider_collisionResult {
        bool collided;
        vec3s penetration;
};

void collider_evaluate(const struct collider *a, const struct collider *b,
                       struct collider_collisionResult *r)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((access (write_only, 3)))
        __attribute__((nonnull));

#endif
