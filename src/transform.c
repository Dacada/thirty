#include <thirty/transform.h>
#include <thirty/util.h>

void transform_init(struct transform *trans, mat4s model) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        component_init(&trans->base, "transform");
        trans->model = model;
}

void transform_initFromFile(struct transform *trans, FILE *f,
                            enum componentType type) {
        assert(type == COMPONENT_TRANSFORM);
        mat4s model;
        sfread(model.raw, sizeof(float), sizeof(model)/sizeof(float), f);
        transform_init(trans, model);
}

void transform_translate(struct transform *const trans, const vec3s delta) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model = glms_translate(trans->model, delta);
}

void transform_translateX(struct transform *const trans, const float delta) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model = glms_translate_x(trans->model, delta);
}

void transform_translateY(struct transform *const trans, const float delta) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model = glms_translate_y(trans->model, delta);
}

void transform_translateZ(struct transform *const trans, const float delta) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model = glms_translate_z(trans->model, delta);
}

void transform_set(struct transform *trans, vec3s value) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model.col[3].x = value.x;
        trans->model.col[3].y = value.y;
        trans->model.col[3].z = value.z;
}

/*
 * Set X component of translation of model matrix to given value.
 */
void transform_setX(struct transform *trans, float value) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model.col[3].x = value;
}

/*
 * Set Y component of translation of model matrix to given value.
 */
void transform_setY(struct transform *trans, float value) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model.col[3].y = value;
}

/*
 * Set Z component of translation of model matrix to given value.
 */
void transform_setZ(struct transform *trans, float value) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model.col[3].z = value;
}

void transform_rotate(struct transform *const trans, const float angle,
                   const vec3s axis) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model = glms_rotate(trans->model, angle, axis);
}

void transform_rotateX(struct transform *trans, float angle) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model = glms_rotate_x(trans->model, angle);
}

void transform_rotateY(struct transform *trans, float angle) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model = glms_rotate_y(trans->model, angle);
}

void transform_rotateZ(struct transform *trans, float angle) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model = glms_rotate_z(trans->model, angle);
}

void transform_rotateMat(struct transform *trans, mat4s rotation) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model = glms_mat4_mul(trans->model, rotation);
}

void transform_scale(struct transform *const trans, const vec3s scale) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        trans->model = glms_scale(trans->model, scale);
}

void transform_free(struct transform *trans) {
        assert(trans->base.type == COMPONENT_TRANSFORM);
        component_free(&trans->base);
}
