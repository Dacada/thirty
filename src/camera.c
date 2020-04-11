#include <camera.h>
#include <util.h>
#include <cglm/struct.h>
#include <math.h>

static const vec3s defaultPosition = { .x=0.0f, .y=0.0f, .z=0.0f };
static const vec3s defaultUp = { .x=0.0f, .y=1.0f, .z=0.0f };
static const vec3s defaultFront = { .x=0.0f, .y=0.0f, .z=-1.0f };
static const float defaultYaw = 0.0f;
static const float defaultPitch = 0.0f;
static const float defaultSpeed = 10.0f;
static const float defaultSensitivity = 0.1f;
static const float defaultZoom = 45.0f;

static void update_camera_vectors(struct camera *const cam) {
        vec3s front;
        front.x = cosf(cam->yaw) * cosf(cam->pitch);
        front.y = sinf(cam->pitch);
        front.z = sinf(cam->yaw) * cosf(cam->pitch);
        cam->front = glms_vec3_normalize(front);

        cam->right = glms_vec3_crossn(cam->front, cam->world_up);
        cam->up = glms_vec3_crossn(cam->right, cam->front);
}

static void init_defaults(struct camera *const cam) {
        cam->front = defaultFront;
        cam->movement_speed = defaultSpeed;
        cam->look_sensitivity = defaultSensitivity;
        cam->zoom_level = defaultZoom;
}

void camera_init(struct camera *const cam,
                 const float width, const float height,
                 const vec3s *const position, const vec3s *const up,
                 const float *const yaw, const float *const pitch) {
        init_defaults(cam);
        cam->width = width;
        cam->height = height;
        cam->yaw = NULLDEFAULT(yaw, defaultYaw);
        cam->pitch = NULLDEFAULT(pitch, defaultPitch);
        cam->position = NULLDEFAULT(position, defaultPosition);
        cam->world_up = NULLDEFAULT(up, defaultUp);
        update_camera_vectors(cam);
}

mat4s camera_viewMatrix(const struct camera *const cam) {
        vec3s tmp = glms_vec3_add(cam->position, cam->front);
        return glms_lookat(cam->position, tmp, cam->up);
}

mat4s camera_projectionMatrix(const struct camera *const cam) {
        float fov = glm_rad(cam->zoom_level);
        float aspect = cam->width / cam->height;
        return glms_perspective(fov, aspect, 0.1f, 100.0f);
}

void camera_move(struct camera *const cam, const enum camera_movement mov,
                 const float deltaTime, const bool freefly) {
        float prev_y = cam->position.y;
        float velocity = cam->movement_speed * deltaTime;
        
        switch (mov) {
        case camera_FORWARD:
                // position += front * velocity
                cam->position = glms_vec3_muladds(
                        cam->front, velocity, cam->position);
                break;
        case camera_BACKWARD:
                // position -= front * velocity
                cam->position = glms_vec3_muladds(
                        cam->front, -velocity, cam->position);
                break;
        case camera_LEFT:
                // position += right * (-velocity)
                cam->position = glms_vec3_muladds(
                        cam->right, -velocity, cam->position);
                break;
        case camera_RIGHT:
                // position += right * velocity
                cam->position = glms_vec3_muladds(
                        cam->right, velocity, cam->position);
                break;
        default:
                break;
        }

        if (!freefly) {
                cam->position.y = prev_y;
        }
}

void camera_look(struct camera *const cam,
                 const float xoffset, const float yoffset,
                 const bool constrain_pitch) {
        cam->yaw += xoffset * cam->look_sensitivity;
        cam->pitch += yoffset * cam->look_sensitivity;

        if (constrain_pitch) {
                cam->pitch = glm_clamp(cam->pitch,
                                       -GLM_PI_2f+0.1f, GLM_PI_2f-0.1f);
        }
        
        update_camera_vectors(cam);
}

void camera_zoom(struct camera *const cam, const float offset) {
        cam->zoom_level -= offset;
        cam->zoom_level = glm_clamp(cam->zoom_level, 1.0f, 45.0f);
}
