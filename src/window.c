#include <window.h>
#include <util.h>
#include <cglm/struct.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#define DEFAULT_WINDOW_CLEARCOLOR { .x=0.2F, .y=0.3F, .z=0.3F, .w=1.0F }
#define STARTING_TIMEDELTA (1.0F/60.0F)

void(*window_onKeyboardInput)(void) = NULL;
void(*window_onKeyboardEvent)(const int, const int, const int) = NULL;
void(*window_onMousePosition)(const double, const double) = NULL;
void(*window_onMouseScroll)(const double) = NULL;
void(*window_onUpdate)(void) = NULL;
void(*window_onDraw)(void) = NULL;
void(*window_onTearDown)(void) = NULL;

vec4s window_clearColor = DEFAULT_WINDOW_CLEARCOLOR;
char *window_title = NULL;

static void onFramebufferSizeChanged(GLFWwindow *const w,
                                     const int width, const int height) {
        (void)w;
        glViewport(0, 0, width, height);
}

static void onKeyboardEvent(GLFWwindow *const w, const int key,
                            const int scancode, const int action,
                            const int mods) {
        (void)w;
        (void)scancode;
        if (window_onKeyboardEvent != NULL) {
                window_onKeyboardEvent(key, action, mods);
        }
}

static void onMousePosition(GLFWwindow *const w, const double xpos,
                            const double ypos) {
        (void)w;
        if (window_onMousePosition != NULL) {
                window_onMousePosition(xpos, ypos);
        }
}

static void onMouseScroll(GLFWwindow *const w, const double xoff,
                          const double yoff) {
        (void)w;
        (void)xoff;
        if (window_onMouseScroll != NULL) {
                window_onMouseScroll(yoff);
        }
}

static void onUpdate(void) {
        if (window_onUpdate != NULL) {
                window_onUpdate();
        }
}

static void onDraw(void) {
        if (window_onDraw != NULL) {
                window_onDraw();
        }
}

static void onTearDown(void) {
        if (window_onTearDown != NULL) {
                window_onTearDown();
        }
}

static GLFWwindow *window = NULL;
static float timeDelta = STARTING_TIMEDELTA;

void window_init(const int width, const int height) {
        glfwInit();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#else
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_FALSE);
#endif

        window = glfwCreateWindow(width, height, "", NULL, NULL);
        if (window == NULL) {
                glfwTerminate();
                die("Failed to create GLFW window.\n");
        }

        glfwMakeContextCurrent(window);

        if (!gladLoadGLLoader((const GLADloadproc)glfwGetProcAddress)) {
                glfwTerminate();
                die("Failed to initialize GLAD.\n");
        }

        glViewport(0, 0, width, height);
        glfwSetFramebufferSizeCallback(window, onFramebufferSizeChanged);
        glfwSetKeyCallback(window, onKeyboardEvent);
        glfwSetCursorPosCallback(window, onMousePosition);
        glfwSetScrollCallback(window, onMouseScroll);
        
        glEnable(GL_DEPTH_TEST);
#ifdef NDEBUG
        glEnable(GL_CULL_FACE);
#else
        glDisable(GL_CULL_FACE);
#endif
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void window_run(void) {
        while (!glfwWindowShouldClose(window)) {
                // Update deltatime
                timeDelta = (const float)glfwGetTime();
                glfwSetTime(0);

                // Update game state
                onUpdate();

                // Clear screen
                glClearColor(window_clearColor.x, window_clearColor.y,
                             window_clearColor.z, window_clearColor.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Draw stuff
                onDraw();

                glfwSwapBuffers(window);

                // Process inputs
                glfwPollEvents();
                if (window_onKeyboardInput != NULL) {
                        window_onKeyboardInput();
                }
        }

        // Clean up
        onTearDown();
        glfwTerminate();
}

float window_timeDelta(void) {
        return timeDelta;
}

bool window_keyPressed(const int key) {
        return glfwGetKey(window, key) == GLFW_PRESS;
}

void window_updateTitle(void) {
        if (window_title == NULL) {
                glfwSetWindowTitle(window, "");
        } else {
                glfwSetWindowTitle(window, window_title);
        }
}

void window_close(void) {
        glfwSetWindowShouldClose(window, 1);
}
