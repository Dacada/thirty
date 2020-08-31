#include <physicalWorld.h>
#include <game.h>
#include <physicalEntity.h>
#include <componentCollection.h>
#include <eventBroker.h>
#include <dsutils.h>
#include <util.h>

#define INTEGRATION_STEP 0.01F
#define GRAVITY (-9.81F)

struct growingArray worlds;

static vec3s getForce(vec3s position, vec3s velocity, float mass) {
        (void)velocity;
        (void)position;
        (void)mass;
        
        vec3s force = GLMS_VEC3_ZERO_INIT;
        force.y = GRAVITY * mass;
        return force;
}

static void integrate(struct physicalWorld *world) {
        growingArray_foreach_START(&world->entities, size_t *, entityIdx)
                struct physicalEntity *entity = componentCollection_compByIdx(
                        *entityIdx);
                physicalEntity_update(entity, INTEGRATION_STEP, getForce);
        growingArray_foreach_END;
}

static void updateWorld(void *registerArgs, void *fireArgs) {
        size_t idx = (size_t)registerArgs;
        struct physicalWorld *world = growingArray_get(&worlds, idx);
        
        struct eventBrokerUpdate *args = fireArgs;
        const float timeDelta = args->timeDelta;

        // Integrate
        world->accumulatedTime += timeDelta;
        while (world->accumulatedTime >= INTEGRATION_STEP) {
                integrate(world);
                world->accumulatedTime -= INTEGRATION_STEP;
        }
        
        // Collision detection
        struct collisionData {
                struct physicalEntity *A;
                struct physicalEntity *B;
                struct collider_collisionResult collision;
        };
        struct growingArray collisions;
        growingArray_init(&collisions, sizeof(struct collisionData), 1);
        for (size_t i=0; i<world->entities.length; i++) {
                size_t *entityAIdx = growingArray_get(&world->entities, i);
                struct physicalEntity *entityA =
                        componentCollection_compByIdx(*entityAIdx);
                
                for (size_t j=i+1; j<world->entities.length; j++) {
                        size_t *entityBIdx = growingArray_get(
                                &world->entities, j);
                        struct physicalEntity *entityB =
                                componentCollection_compByIdx(*entityBIdx);
                        
                        struct collider_collisionResult collision;
                        collider_evaluate(&entityA->collider,
                                          &entityB->collider,
                                          &collision);
                        if (collision.collided) {
                                struct collisionData *data =
                                        growingArray_append(&collisions);
                                data->A = entityA;
                                data->B = entityB;
                                data->collision = collision;
                        }
                }
        }
        
        // Collision resolution
        growingArray_foreach_START(&collisions, struct collisionData *, data)
                struct scene *scene = game_getCurrentScene(data->A->base.game);

                // Move objects away so they don't collide anymore
                vec3s movement;
                if (data->A->kinetic || data->B->kinetic) {
                        movement = data->collision.penetration;
                } else {
                        static const float half = 0.5F;
                        movement = glms_vec3_scale(
                                data->collision.penetration, half);
                }
                if (!data->A->kinetic) {
                        struct object *objectA = scene_getObjectFromIdx(
                                scene, data->A->base.object);
                        struct transform *transA = object_getComponent(
                                objectA, COMPONENT_TRANSFORM);
                        vec3s positionA = glms_vec3(transA->model.col[3]);
                        positionA = glms_vec3_sub(positionA, movement);
                        transA->model.col[3] = glms_vec4(positionA, 1.0F);
                }
                if (!data->B->kinetic) {
                        struct object *objectB = scene_getObjectFromIdx(
                                scene, data->B->base.object);
                        struct transform *transB = object_getComponent(
                                objectB, COMPONENT_TRANSFORM);
                        vec3s positionB = glms_vec3(transB->model.col[3]);
                        positionB = glms_vec3_add(positionB, movement);
                        transB->model.col[3] = glms_vec4(positionB, 1.0F);
                }

                vec3s momentumA = data->A->momentum;
                vec3s momentumB = data->B->momentum;
                float massA = data->A->mass;
                float massB = data->B->mass;
                vec3s dp = glms_vec3_sub(
                        glms_vec3_scale(momentumB, massA),
                        glms_vec3_scale(momentumA, massB));
                dp = glms_vec3_scale(dp, 1.0F/(massA+massB));
                if (!data->A->kinetic) {
                        data->A->momentum = glms_vec3_add(momentumA, dp);
                }
                if (!data->B->kinetic) {
                        data->B->momentum = glms_vec3_sub(momentumB, dp);
                }
        growingArray_foreach_END;
        growingArray_destroy(&collisions);
}

void physicalWorld_startup(void) {
        growingArray_init(&worlds, sizeof(struct physicalWorld), 1);
}

struct physicalWorld *physicalWorld_create(void) {
        struct physicalWorld *world = growingArray_append(&worlds);
        world->idx = worlds.length - 1;
        eventBroker_register(updateWorld, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_UPDATE, (void*)world->idx);
        return world;
}

void physicalWorld_init(struct physicalWorld *const world) {
        growingArray_init(&world->entities, sizeof(size_t), 1);
        world->accumulatedTime = 0;
}

void physicalWorld_addEntity(struct physicalWorld *const world,
                             const size_t idx) {
        size_t *i = growingArray_append(&world->entities);
        *i = idx;
}

void physicalWorld_free(struct physicalWorld *world) {
        growingArray_destroy(&world->entities);
}

void physicalWorld_shutdown(void) {
        growingArray_foreach_START(&worlds, struct physicalWorld *, world)
                growingArray_destroy(&world->entities);
        growingArray_foreach_END;
        growingArray_destroy(&worlds);
}
