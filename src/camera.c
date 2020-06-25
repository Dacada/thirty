#include <camera.h>
#include <util.h>
#include <cglm/struct.h>
#include <math.h>

static const vec3s defaultPosition = { .x=0.0F, .y=0.0F, .z=0.0F };
static const vec3s defaultWorldUp = { .x=0.0F, .y=1.0F, .z=0.0F };
static const vec3s defaultFront = { .x=0.0F, .y=0.0F, .z=-1.0F };
static const float defaultYaw = 0.0F;
static const float defaultPitch = 0.0F;
static const float defaultSpeed = 10.0F;
static const float defaultSensitivity = 0.1F;
static const float defaultZoom = 45.0F;
static const float defaultNear = 0.1F;
static const float defaultFar = 100.0F;

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
                 const vec3s *const position, const vec3s *const worldup,
                 const float *const yaw, const float *const pitch,
                 const float *const near, const float *const far) {
        init_defaults(cam);
        cam->width = width;
        cam->height = height;
        cam->yaw = NULLDEFAULT(yaw, defaultYaw);
        cam->pitch = NULLDEFAULT(pitch, defaultPitch);
        cam->position = NULLDEFAULT(position, defaultPosition);
        cam->world_up = NULLDEFAULT(worldup, defaultWorldUp);
        cam->near = NULLDEFAULT(near, defaultNear);
        cam->far = NULLDEFAULT(far, defaultFar);
        update_camera_vectors(cam);
}

mat4s camera_viewMatrix(const struct camera *const cam) {
        const vec3s tmp = glms_vec3_add(cam->position, cam->front);
        return glms_lookat(cam->position, tmp, cam->up);
}

mat4s camera_projectionMatrix(const struct camera *const cam) {
        const float fov = glm_rad(cam->zoom_level);
        const float aspect = cam->width / cam->height;
        return glms_perspective(fov, aspect, cam->near, cam->far);
}

void camera_move(struct camera *const cam, const enum camera_movement mov,
                 const float deltaTime, const bool freefly) {
        const float prev_y = cam->position.y;
        const float velocity = cam->movement_speed * deltaTime;
        
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

void camera_look(struct camera *const cam, const float xoffset,
                 const float yoffset, const bool constrain_pitch) {
        cam->yaw += xoffset * cam->look_sensitivity;
        cam->pitch += yoffset * cam->look_sensitivity;

        if (constrain_pitch) {
                cam->pitch = glm_clamp(
                        cam->pitch,
                        -GLM_PI_2f+CAMERA_PITCH_LIMIT_OFFSET,
                        GLM_PI_2f-CAMERA_PITCH_LIMIT_OFFSET);
        }
        
        update_camera_vectors(cam);
}

void camera_zoom(struct camera *const cam, const float offset) {
        cam->zoom_level -= offset;
        cam->zoom_level = glm_clamp(
                cam->zoom_level, CAMERA_MINIMUM_ZOOM, CAMERA_MAXIMUM_ZOOM);
}
