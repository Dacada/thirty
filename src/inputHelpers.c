#include <inputHelpers.h>
#include <util.h>
#include <camera.h>
#include <object.h>
#include <cglm/struct.h>

void fpsCameraController_init(struct fpsCameraController *const ctrl,
                              const float move, const float look,
                              const struct object *const camera) {
        ctrl->freefly = false;
        ctrl->move_sensitivity = move;
        ctrl->look_sensitivity = look;
        ctrl->camera_obj = camera;
        ctrl->camera = (struct fpsCamera *)camera->camera;

        assert(ctrl->camera != NULL);
        assert(ctrl->camera->base.type == CAMERA_FPS);
}

static mat4s get_model(const struct object *obj) {
        mat4s model = obj->model;
        while (obj->parent != NULL) {
                model = glms_mat4_mul(obj->parent->model, model);
                obj = obj->parent;
        }
        return model;
}

void fpsCameraController_move(const struct fpsCameraController *const ctrl,
                              const vec2s direction, const float timeDelta) {
        const float move = ctrl->move_sensitivity * timeDelta;

        static const vec3s worldUp = {.x=0, .y=1, .z=0};
        static const vec3s worldForward = {.x=0, .y=0, .z=-1};

        mat4s model = get_model(ctrl->camera_obj);
        mat4s r;
        vec3s s;
        glms_decompose_rs(model, &r, &s);
        const vec3s cameraUp = glms_vec3_rotate_m4(r, worldUp);
        const vec3s cameraForward = glms_vec3_rotate_m4(r, worldForward);

        vec3s forward = glms_vec3_rotate(
                cameraForward, ctrl->camera->yaw, cameraUp);
        vec3s right = glms_vec3_cross(forward, cameraUp);
        if (ctrl->freefly) {
                forward = glms_vec3_rotate(
                        forward, ctrl->camera->pitch, right);
        }
        forward = glms_vec3_normalize(forward);
        right = glms_vec3_normalize(right);

        ctrl->camera->position = glms_vec3_muladds(
                forward, direction.y * move, ctrl->camera->position);
        ctrl->camera->position = glms_vec3_muladds(
                right, direction.x * move, ctrl->camera->position);
}

void fpsCameraController_look(const struct fpsCameraController *const ctrl,
                              const vec2s direction, const float timeDelta) {
        const float look = ctrl->look_sensitivity * timeDelta;
        ctrl->camera->yaw += direction.x * look;
        ctrl->camera->pitch += direction.y * look;
}