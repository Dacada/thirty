#ifndef PHYSICAL_ENTITY_H
#define PHYSICAL_ENTITY_H

#include <component.h>
#include <collider.h>
#include <cglm/struct.h>

struct physicalEntity {
        struct component base;

        bool kinetic;
        
        float mass;
        float inverseMass;
        
        vec3s momentum;
        vec3s velocity;

        struct collider collider;
};

typedef vec3s(*getForceCb)(vec3s, vec3s, float);

void physicalEntity_init(struct physicalEntity *phys, float mass,
                         const char *name)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

size_t physicalEntity_initFromFile(struct physicalEntity *phys, FILE *f,
                                   enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

void physicalEntity_setColliderSphere(struct physicalEntity *phys,
                                      float radius)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void physicalEntity_setColliderPlane(struct physicalEntity *phys,
                                     vec3s normal, float distance)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void physicalEntity_setColliderAABB(struct physicalEntity *phys,
                                    vec3s halfDistances)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void physicalEntity_update(struct physicalEntity *phys, float dt,
                           getForceCb getForce)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void physicalEntity_free(struct physicalEntity *phys)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
