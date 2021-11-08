#include <thirty/inputHelpers.h>
#include <thirty/util.h>

void fpsCameraController_init(struct fpsCameraController *const ctrl,
                              struct scene *const scene,
                              const float move, const float look) {
        ctrl->freefly = false;
        ctrl->move_sensitivity = move;
        ctrl->look_sensitivity = look;
        ctrl->scene = scene;
}

void fpsCameraController_move(const struct fpsCameraController *const ctrl,
                              const vec2s direction, const float timeDelta,
                              struct object *const camera_obj) {
        struct camera_fps *camera = object_getComponent(camera_obj, COMPONENT_CAMERA);
        const float move = ctrl->move_sensitivity * timeDelta;

        static const vec3s worldUp = {.x=0, .y=1, .z=0};
        static const vec3s worldForward = {.x=0, .y=0, .z=-1};

        mat4s model = scene_getObjectAbsoluteTransform(ctrl->scene, camera_obj);
        mat4s r;
        vec3s s;
        glms_decompose_rs(model, &r, &s);
        const vec3s cameraUp = glms_vec3_rotate_m4(r, worldUp);
        const vec3s cameraForward = glms_vec3_rotate_m4(r, worldForward);

        vec3s forward = glms_vec3_rotate(
                cameraForward, camera->yaw, cameraUp);
        vec3s right = glms_vec3_cross(forward, cameraUp);
        if (ctrl->freefly) {
                forward = glms_vec3_rotate(
                        forward, camera->pitch, right);
        }
        forward = glms_vec3_normalize(forward);
        right = glms_vec3_normalize(right);

        camera->position = glms_vec3_muladds(
                forward, direction.y * move, camera->position);
        camera->position = glms_vec3_muladds(
                right, direction.x * move, camera->position);
}

void fpsCameraController_look(const struct fpsCameraController *const ctrl,
                              const vec2s direction, const float timeDelta,
                              struct object *const camera_obj) {
        struct camera_fps *camera = object_getComponent(camera_obj, COMPONENT_CAMERA);
        const float look = ctrl->look_sensitivity * timeDelta;
        camera->yaw += direction.x * look;
        camera->pitch += direction.y * look;
        camera->pitch = glm_clamp(camera->pitch,
                                  -GLM_PI_2f, GLM_PI_2f);
}
