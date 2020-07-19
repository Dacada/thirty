#include <scene.h>
#include <window.h>
#include <util.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TITLE_BUFFER_SIZE 256
#define FRAME_PERIOD_FPS_REFRESH 10
#define CAMERA_PITCH_LIMIT_OFFSET 0.1F

static const float movement_speed = 10.0F;
static const float look_sensitivity = 2.0F;

static struct scene *scene;

static char title[TITLE_BUFFER_SIZE];
static bool freefly = false;

enum direction {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
};

static struct object *find_camera(struct object *const object) {
        if (object->camera != NULL) {
                return object;
        }

        for (unsigned i=0; i<object->nchildren; i++) {
                struct object *ret = find_camera(object->children[i]);
                if (ret != NULL) {
                        return ret;
                }
        }

        return NULL;
}

static struct object *get_camera(void) {
        static bool first = true;
        static struct object *camera;
        if (first) {
                camera = find_camera(&scene->root);
                first = false;
        }
        return camera;
}

static void processKeyboardInput(void) {
        struct object *const camera = get_camera();
        const float timeDelta = window_timeDelta();

        const float look = look_sensitivity * timeDelta;
        const float move = movement_speed * timeDelta;
        
        if (window_keyPressed(GLFW_KEY_W)) {
                object_rotateX(camera, look);
        }
        if (window_keyPressed(GLFW_KEY_S)) {
                object_rotateX(camera, -look);
        }
        if (window_keyPressed(GLFW_KEY_A)) {
                object_rotateY(camera, look);
        }
        if (window_keyPressed(GLFW_KEY_D)) {
                object_rotateY(camera, -look);
        }
        if (window_keyPressed(GLFW_KEY_Q)) {
                object_rotateZ(camera, look);
        }
        if (window_keyPressed(GLFW_KEY_E)) {
                object_rotateZ(camera, -look);
        }
        
        if (window_keyPressed(GLFW_KEY_LEFT)) {
                object_translateX(camera, -move);
        }
        if (window_keyPressed(GLFW_KEY_RIGHT)) {
                object_translateX(camera, move);
        }
        if (window_keyPressed(GLFW_KEY_PAGE_UP)) {
                object_translateY(camera, move);
        }
        if (window_keyPressed(GLFW_KEY_PAGE_DOWN)) {
                object_translateY(camera, -move);
        }
        if (window_keyPressed(GLFW_KEY_UP)) {
                object_translateZ(camera, -move);
        }
        if (window_keyPressed(GLFW_KEY_DOWN)) {
                object_translateZ(camera, move);
        }
}

static void processKeyboardEvent(const int key, const int action,
                                 const int modifiers) {
        (void)modifiers;
        if (action == GLFW_PRESS) {
                if (key == GLFW_KEY_ESCAPE) {
                        window_close();
                } else if (key == GLFW_KEY_F) {
                        freefly = !freefly;
                }
        }
}

static void draw(void) {
        static unsigned count = 0;
        if (count == FRAME_PERIOD_FPS_REFRESH) {
                const int fps = (const int)(1.0F/window_timeDelta());
                snprintf(title, TITLE_BUFFER_SIZE, "[%d] Temple (%s)", fps,
                         freefly ? "Freefly camera" : "FPS camera");
                window_updateTitle();
                count = 0;
        } else {
                count++;
        }

        scene_draw(scene);

#ifndef NDEBUG
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
                fprintf(stderr, "OpenGL Error: %u", error);
        }
#endif
}

static void freeScene(void) {
        scene_free(scene);
        free(scene);
}

int main(void) {
        window_init(SCREEN_WIDTH, SCREEN_HEIGHT);
        window_title = title;

        scene = smalloc(sizeof(*scene));
        scene_initFromFile(scene, "scene");

        window_onKeyboardInput = processKeyboardInput;
        window_onKeyboardEvent = processKeyboardEvent;
        window_onDraw = draw;
        window_onTearDown = freeScene;
        
        window_run();
        
        return 0;
}
