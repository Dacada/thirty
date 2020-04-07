#include <object.h>
#include <shader.h>
#include <camera.h>
#include <window.h>
#include <cglm/cglm.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

static unsigned int shader;

static struct object cubes[10];

static const vec3s cubePositions[] = {
  { .x= 0.0f, .y= 0.0f, .z=  0.0f },
  { .x= 2.0f, .y= 5.0f, .z=-15.0f },
  { .x=-1.5f, .y=-2.2f, .z=- 2.5f },
  { .x=-3.8f, .y=-2.0f, .z=-12.3f },
  { .x= 2.4f, .y=-0.4f, .z=- 3.5f },
  { .x=-1.7f, .y= 3.0f, .z=- 7.5f },
  { .x= 1.3f, .y=-2.0f, .z=- 2.5f },
  { .x= 1.5f, .y= 2.0f, .z=- 2.5f },
  { .x= 1.5f, .y= 0.2f, .z=- 1.5f },
  { .x=-1.3f, .y= 1.0f, .z=- 1.5f }
};

static const char *cubeTextures[] = {
        "container",
        "awesomeface"
};

static const vec3s startingCameraPosition = {
        .x=0.0f, .y=0.0f, .z=3.0f
};
static struct camera cam;

static char title[256];
static bool freefly = false;

static void processKeyboardInput(void) {
        float deltaTime = window_timeDelta();
        
        if (window_keyPressed(GLFW_KEY_W)) {
                camera_move(&cam, camera_FORWARD, deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_S)) {
                camera_move(&cam, camera_BACKWARD, deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_A)) {
                camera_move(&cam, camera_LEFT, deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_D)) {
                camera_move(&cam, camera_RIGHT, deltaTime, freefly);
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

static void processMousePosition(const double xpos, const double ypos) {
        static bool first = true;
        static vec2s last;

        vec2s pos = {
                .x=(float)xpos,
                .y=(float)ypos
        };

        if (first) {
                last = pos;
                first = false;
        } else {
                vec2s offset = {
                        .x=pos.x - last.x,
                        .y=last.y - pos.y
                };
                last = pos;
                camera_look(&cam, offset.x, offset.y, true);
        }
}

static void processMouseScroll(const double offset) {
        camera_zoom(&cam, (float)offset);
}

static void draw(void) {
        static unsigned count = 0;
        if (count == 10) {
                int fps = (int)(1.0f/window_timeDelta());
                snprintf(title, 256, "[%d] Boxes (%s)", fps,
                         freefly ? "Freefly camera" : "FPS camera");
                window_updateTitle();
                count = 0;
        } else {
                count++;
        }
        
        for (int i=0; i<10; i++) {
                struct object *cube = &cubes[i];
                object_draw(cube, &cam, shader);
        }

#ifndef NDEBUG
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
                fprintf(stderr, "OpenGL Error: %u", error);
        }
#endif
}

int main(void) {
        object_initModule();
        
        camera_init(&cam, SCREEN_WIDTH, SCREEN_HEIGHT,
                    &startingCameraPosition, NULL, NULL, NULL);
        cam.look_sensitivity = 0.01f;

        window_init(SCREEN_WIDTH, SCREEN_HEIGHT);
        window_title = title;
        
        shader = shader_new("mix_two_textures", "simple");
        shader_use(shader);
        shader_setInt(shader, "texture0", 0);
        shader_setInt(shader, "texture1", 1);

        for (int i=0; i<10; i++) {
                struct object *cube = &cubes[i];
                object_initFromFile(&cube, "test", true);
                
                object_setTextures(cube, cubeTextures, 2);
                
                static const vec3s rot_axis = (vec3s){
                        .x=1.0f,
                        .y=0.3f,
                        .z=0.5f
                };
                static const vec3s scale = (vec3s){
                        .x=0.5f,
                        .y=0.5f,
                        .z=0.5f
                };
                float angle = (float)i * 20.0f;
                object_translate(cube, cubePositions[i]);
                object_rotate(cube, angle, rot_axis);
                object_scale(cube, scale);
        }

        window_onKeyboardInput = processKeyboardInput;
        window_onKeyboardEvent = processKeyboardEvent;
        window_onMousePosition = processMousePosition;
        window_onMouseScroll = processMouseScroll;
        window_onDraw = draw;
        
        window_run();

        for (int i=0; i<10; i++) {
                //object_tearDown(&cubes[i]);
        }
        return 0;
}
