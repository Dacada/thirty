#include <collider.h>
#include <util.h>

static void evaluateAlwaysFalseCollision(struct collider_collisionResult *r) {
        r->collided = false;
        r->penetration = GLMS_VEC3_ZERO;
}

static void evaluateSphereSphereCollision(const struct collider_sphere *a,
                                          const struct collider_sphere *b,
                                          struct collider_collisionResult *r) {
        float distance = glms_vec3_distance(a->center, b->center);
        float touchingDistance = a->radius + b->radius;

        r->collided = distance <= touchingDistance;

        if (!r->collided) {
                r->penetration = GLMS_VEC3_ZERO;
                return;
        }

        vec3s AtoB = glms_vec3_sub(a->center, b->center);
        vec3s rA = glms_vec3_scale(glms_vec3_normalize(AtoB), a->radius);
        vec3s rB = glms_vec3_scale(glms_vec3_normalize(AtoB), b->radius);

        r->penetration = glms_vec3_sub(glms_vec3_sub(AtoB, rA), rB);
}

static void evaluatePlanePlaneCollision(const struct collider_plane *a,
                                        const struct collider_plane *b,
                                        struct collider_collisionResult *r) {
        r->penetration = glms_vec3_cross(a->normal, b->normal);
        r->collided = !glms_vec3_eq_eps(r->penetration, 0.0F) ||
                fabsf(a->distance - b->distance) <= FLT_EPSILON;
}

static void evaluateSpherePlaneCollision(const struct collider_sphere *a,
                                         const struct collider_plane *b,
                                         struct collider_collisionResult *r) {
        float planeToCircle = glms_vec3_dot(a->center, b->normal);
        planeToCircle -= b->distance;
        float penetrationMagnitude = fabsf(planeToCircle) - a->radius;

        r->collided = penetrationMagnitude <= 0;
        if (!r->collided) {
                return;
        }

        r->penetration = glms_vec3_scale(b->normal, penetrationMagnitude);
}

static void evaluateSphereAABBCollision(const struct collider_sphere *a,
                                        const struct collider_aabb *b,
                                        struct collider_collisionResult *r) {
        vec3s AABBClosestPointToSphereCenter = {
                .x = max(b->center.x - b->halfDistances.x,
                         min(a->center.x,
                             b->center.x + b->halfDistances.x)),
                .y = max(b->center.y - b->halfDistances.y,
                         min(a->center.y,
                             b->center.y + b->halfDistances.y)),
                .z = max(b->center.z - b->halfDistances.z,
                         min(a->center.z,
                             b->center.z + b->halfDistances.z)),
        };

        r->collided = glms_vec3_distance2(
                AABBClosestPointToSphereCenter,
                a->center) < a->radius * a->radius;

        if (!r->collided) {
                return;
        }

        vec3s sphereCenterToAABBCenter = glms_vec3_sub(b->center, a->center);
        vec3s sphereCenterToSphereClosestPointToAABBCenter =
                glms_vec3_scale(glms_vec3_normalize(sphereCenterToAABBCenter),
                                a->radius);
        vec3s sphereClosestPointToAABBCenter =
                glms_vec3_add(a->center,
                              sphereCenterToSphereClosestPointToAABBCenter);
        r->penetration = glms_vec3_sub(sphereClosestPointToAABBCenter,
                                       AABBClosestPointToSphereCenter);
}

static void evaluatePlaneAABBCollision(const struct collider_plane *a,
                                        const struct collider_aabb *b,
                                        struct collider_collisionResult *r) {
        float AABBHalfDistancesProjectedOntoPlaneNormal =
                glms_vec3_dot(b->halfDistances, a->normal);
        float AABBCenterProjectedOntoPlaneNormal =
                glms_vec3_dot(b->center, a->normal);
        float distanceAABBCenterPlane =
                fabsf(AABBCenterProjectedOntoPlaneNormal - a->distance);
        float differenceDistanceAABBCenterToPlaneAndAABBRadius =
                distanceAABBCenterPlane -
                AABBHalfDistancesProjectedOntoPlaneNormal;
        r->collided = differenceDistanceAABBCenterToPlaneAndAABBRadius <= 0;

        if (!r->collided) {
                return;
        }
        r->penetration = glms_vec3_scale(
                a->normal, differenceDistanceAABBCenterToPlaneAndAABBRadius);
}

static void evaluateAABBAABBCollision(const struct collider_aabb *a,
                                        const struct collider_aabb *b,
                                        struct collider_collisionResult *r) {
        float a_xmin = a->center.x - a->halfDistances.x;
        float a_xmax = a->center.x + a->halfDistances.x;
        float b_xmin = b->center.x - b->halfDistances.x;
        float b_xmax = b->center.x + b->halfDistances.x;
        
        float a_ymin = a->center.y - a->halfDistances.y;
        float a_ymax = a->center.y + a->halfDistances.y;
        float b_ymin = b->center.y - b->halfDistances.y;
        float b_ymax = b->center.y + b->halfDistances.y;
        
        float a_zmin = a->center.z - a->halfDistances.z;
        float a_zmax = a->center.z + a->halfDistances.z;
        float b_zmin = b->center.z - b->halfDistances.z;
        float b_zmax = b->center.z + b->halfDistances.z;

        bool a_lef_b =
                a_xmin < b_xmin &&
                a_xmin < b_xmax &&
                a_xmax > b_xmin &&
                a_xmax < b_xmax;
        bool a_inx_b =
                a_xmin > b_xmin &&
                a_xmin < b_xmax &&
                a_xmax > b_xmin &&
                a_xmax < b_xmax;
        bool a_rig_b =
                a_xmin > b_xmin &&
                a_xmin < b_xmax &&
                a_xmax > b_xmin &&
                a_xmax > b_xmax;

        bool a_bel_b =
                a_ymin < b_ymin &&
                a_ymin < b_ymax &&
                a_ymax > b_ymin &&
                a_ymax < b_ymax;
        bool a_iny_b =
                a_ymin > b_ymin &&
                a_ymin < b_ymax &&
                a_ymax > b_ymin &&
                a_ymax < b_ymax;
        bool a_top_b =
                a_ymin > b_ymin &&
                a_ymin < b_ymax &&
                a_ymax > b_ymin &&
                a_ymax > b_ymax;

        bool a_beh_b =
                a_zmin < b_zmin &&
                a_zmin < b_zmax &&
                a_zmax > b_zmin &&
                a_zmax < b_zmax;
        bool a_inz_b =
                a_zmin > b_zmin &&
                a_zmin < b_zmax &&
                a_zmax > b_zmin &&
                a_zmax < b_zmax;
        bool a_fro_b =
                a_zmin > b_zmin &&
                a_zmin < b_zmax &&
                a_zmax > b_zmin &&
                a_zmax > b_zmax;

        r->collided =
                (a_lef_b || a_inx_b || a_rig_b) &&
                (a_bel_b || a_iny_b || a_top_b) &&
                (a_beh_b || a_inz_b || a_fro_b);

        if (r->collided) {
                vec3s v1 = GLMS_VEC3_ZERO;
                vec3s v2 = GLMS_VEC3_ZERO;
                
                if (a_lef_b) {
                        v1.x = a_xmax;
                        v2.x = b_xmin;
                } else if (a_rig_b) {
                        v1.x = a_xmin;
                        v2.x = b_xmax;
                }
                
                if (a_bel_b) {
                        v1.y = a_ymax;
                        v2.y = b_ymin;
                } else if (a_top_b) {
                        v1.y = a_ymin;
                        v2.y = b_ymax;
                }
                
                if (a_beh_b) {
                        v1.z = a_zmax;
                        v2.z = b_zmin;
                } else if (a_fro_b) {
                        v1.z = a_zmin;
                        v2.z = b_zmax;
                }
                
                r->penetration = glms_vec3_sub(v1, v2);
        }
}

void collider_evaluate(const struct collider *a, const struct collider *b,
                       struct collider_collisionResult *r) {
        switch (a->type) {
        case COLLIDER_SPHERE:
                switch (b->type) {
                case COLLIDER_SPHERE:
                        evaluateSphereSphereCollision(
                                &a->sphere, &b->sphere, r);
                        break;
                case COLLIDER_PLANE:
                        evaluateSpherePlaneCollision(
                                &a->sphere, &b->plane, r);
                        break;
                case COLLIDER_AABB:
                        evaluateSphereAABBCollision(
                                &a->sphere, &b->aabb, r);
                        break;
                case COLLIDER_NONE:
                default:
                        evaluateAlwaysFalseCollision(r);
                        break;
                }
                break;
        case COLLIDER_PLANE:
                switch(b->type) {
                case COLLIDER_SPHERE:
                        evaluateSpherePlaneCollision(
                                &b->sphere, &a->plane, r);
                        break;
                case COLLIDER_PLANE:
                        evaluatePlanePlaneCollision(
                                &a->plane, &b->plane, r);
                        break;
                case COLLIDER_AABB:
                        evaluatePlaneAABBCollision(
                                &a->plane, &b->aabb, r);
                        break;
                case COLLIDER_NONE:
                default:
                        evaluateAlwaysFalseCollision(r);
                        break;
                }
                break;
        case COLLIDER_AABB:
                switch (b->type) {
                case COLLIDER_SPHERE:
                        evaluateSphereAABBCollision(
                                &b->sphere, &a->aabb, r);
                        break;
                case COLLIDER_PLANE:
                        evaluatePlaneAABBCollision(
                                &b->plane, &a->aabb, r);
                        break;
                case COLLIDER_AABB:
                        evaluateAABBAABBCollision(
                                &a->aabb, &b->aabb, r);
                        break;
                case COLLIDER_NONE:
                        break;
                default:
                        evaluateAlwaysFalseCollision(r);
                        break;
                }
                break;
        case COLLIDER_NONE:
        default:
                evaluateAlwaysFalseCollision(r);
                break;
        }
}
