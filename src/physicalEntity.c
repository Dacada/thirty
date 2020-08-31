#include <game.h>
#include <object.h>
#include <physicalEntity.h>
#include <transform.h>

void physicalEntity_init(struct physicalEntity *const phys,
                         const float mass,
                         const char *const name) {
        component_init(&phys->base, name);

        phys->kinetic = false;
        
        phys->mass = mass;
        phys->inverseMass = 1.0F / mass;
        
        phys->momentum = GLMS_VEC3_ZERO;
        phys->velocity = GLMS_VEC3_ZERO;

        phys->collider.type = COLLIDER_NONE;
}

size_t physicalEntity_initFromFile(struct physicalEntity *phys, FILE *f,
                                   enum componentType type) {
        // TODO
        (void)phys;
        (void)f;
        (void)type;
        return 0;
}

void physicalEntity_setColliderSphere(struct physicalEntity *const phys,
                                      const float radius) {
        phys->collider.type = COLLIDER_SPHERE;
        phys->collider.sphere.radius = radius;
        
        struct scene *scene = game_getCurrentScene(phys->base.game);
        struct object *object = scene_getObjectFromIdx(
                scene, phys->base.object);
        struct transform *trans = object_getComponent(
                object, COMPONENT_TRANSFORM);
        phys->collider.sphere.center = glms_vec3(trans->model.col[3]);
}

void physicalEntity_setColliderPlane(struct physicalEntity *const phys,
                                     const vec3s normal,
                                     const float distance) {
        phys->collider.type = COLLIDER_PLANE;
        phys->collider.plane.normal = normal;
        phys->collider.plane.distance = distance;
}

void physicalEntity_setColliderAABB(struct physicalEntity *phys,
                                    vec3s halfDistances) {
        phys->collider.type = COLLIDER_AABB;
        phys->collider.aabb.halfDistances = halfDistances;
        
        struct scene *scene = game_getCurrentScene(phys->base.game);
        struct object *object = scene_getObjectFromIdx(
                scene, phys->base.object);
        struct transform *trans = object_getComponent(
                object, COMPONENT_TRANSFORM);
        phys->collider.aabb.center = glms_vec3(trans->model.col[3]);
}

void physicalEntity_update(struct physicalEntity *const phys, const float dt,
                           const getForceCb getForce) {
        if (phys->kinetic) {
                return;
        }
        
        struct scene *scene = game_getCurrentScene(phys->base.game);
        struct object *object = scene_getObjectFromIdx(
                scene, phys->base.object);
        struct transform *trans = object_getComponent(
                object, COMPONENT_TRANSFORM);

        vec3s position = glms_vec3(trans->model.col[3]);
        vec3s velocity = phys->velocity;
        vec3s momentum = phys->momentum;
        vec3s force = getForce(position, velocity, phys->mass);
        vec3s dp = glms_vec3_scale(force, dt);
        momentum = glms_vec3_add(momentum, dp);
        velocity = glms_vec3_scale(momentum, phys->inverseMass);
        vec3s dx = glms_vec3_scale(velocity, dt);
        position = glms_vec3_add(position, dx);
        phys->momentum = momentum;
        phys->velocity = velocity;
        trans->model.col[3] = glms_vec4(position, 1.0F);

        switch (phys->collider.type) {
        case COLLIDER_SPHERE:
                phys->collider.sphere.center = position;
                break;
        case COLLIDER_PLANE:
                // TODO
                break;
        case COLLIDER_AABB:
                phys->collider.aabb.center = position;
                break;
        case COLLIDER_NONE:
        default:
                break;
        }
}

void physicalEntity_free(struct physicalEntity *const phys) {
        component_free(&phys->base);
}
