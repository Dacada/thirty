#include <window.h>
#include <util.h>
#include <eventBroker.h>
#include <cglm/struct.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#define DEFAULT_WINDOW_CLEARCOLOR { .x=0.2F, .y=0.3F, .z=0.3F, .w=1.0F }
#define STARTING_TIMEDELTA (1.0F/60.0F)

vec4s window_clearColor = DEFAULT_WINDOW_CLEARCOLOR;

static void onFramebufferSizeChanged(void *vargs) {
        struct eventBrokerWindowResized *args = vargs;
        const int width = args->width;
        const int height = args->height;
        glViewport(0, 0, width, height);
}

static void eventFire_windowResized(GLFWwindow *const w,
                                    const int width, const int height) {
        (void)w;
        struct eventBrokerWindowResized args = {
                .width = width,
                .height = height,
        };
        eventBroker_fire(EVENT_BROKER_WINDOW_RESIZED, &args);
}

static void eventFire_keyboardEvent(GLFWwindow *const w, const int key,
                                    const int scancode, const int action,
                                    const int mods) {
        (void)w;
        (void)scancode;
        struct eventBrokerKeyboardEvent args = {
                .key = key,
                .action = action,
                .modifiers = mods,
        };
        eventBroker_fire(EVENT_BROKER_KEYBOARD_EVENT, &args);
}

static void eventFire_keyboardInput(void) {
        eventBroker_fire(EVENT_BROKER_KEYBOARD_INPUT, NULL);
}

static void eventFire_mousePosition(GLFWwindow *const w,
                                    const double xpos, const double ypos) {
        (void)w;
        struct eventBrokerMousePosition args = {
                .xpos = xpos,
                .ypos = ypos,
        };
        eventBroker_fire(EVENT_BROKER_MOUSE_POSITION, &args);
}

static void eventFire_mouseScroll(GLFWwindow *const w,
                                  const double xoff, const double yoff) {
        (void)w;
        (void)xoff;
        struct eventBrokerMouseScroll args = {
                .amount = yoff,
        };
        eventBroker_fire(EVENT_BROKER_MOUSE_SCROLL, &args);
}

static void eventFire_update(void) {
        eventBroker_fire(EVENT_BROKER_UPDATE, NULL);
}

static void eventFire_draw(void) {
        eventBroker_fire(EVENT_BROKER_DRAW, NULL);
}

static void eventFire_tearDown(void) {
        eventBroker_fire(EVENT_BROKER_TEAR_DOWN, NULL);
}

static GLFWwindow *window = NULL;
static float timeDelta = STARTING_TIMEDELTA;

void window_init(const int width, const int height) {
        eventBroker_startup();
        
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
        glfwSetFramebufferSizeCallback(window, eventFire_windowResized);
        glfwSetKeyCallback(window, eventFire_keyboardEvent);
        glfwSetCursorPosCallback(window, eventFire_mousePosition);
        glfwSetScrollCallback(window, eventFire_mouseScroll);

        eventBroker_register(onFramebufferSizeChanged, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_WINDOW_RESIZED);
        
        glEnable(GL_DEPTH_TEST);
#ifdef NDEBUG
        glEnable(GL_CULL_FACE);
#else
        glDisable(GL_CULL_FACE);
#endif
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

#ifndef NDEBUG
#define FRAMES_TIME_INFO 60

static void updateTimingInformation(double deltaTime, double updateTime,
                                    double drawTime, double buffSwapTime,
                                    double otherEventsTime) {
        
        static double deltaTimes[FRAMES_TIME_INFO];
        static double updateTimes[FRAMES_TIME_INFO];
        static double drawTimes[FRAMES_TIME_INFO];
        static double buffSwapTimes[FRAMES_TIME_INFO];
        static double otherEventsTimes[FRAMES_TIME_INFO];
        static unsigned current = 0;

        deltaTimes[current] = deltaTime;
        updateTimes[current] = updateTime;
        drawTimes[current] = drawTime - updateTime;
        buffSwapTimes[current] = buffSwapTime - drawTime;
        otherEventsTimes[current] = otherEventsTime - buffSwapTime;
        current++;

        if (current >= FRAMES_TIME_INFO) {
                current = 0;
                
                double avgDeltaTimes = 0;
                double avgUpdateTimes = 0;
                double avgDrawTimes = 0;
                double avgBuffSwapTimes = 0;
                double avgOtherEventsTimes = 0;

                for (unsigned i=0; i<FRAMES_TIME_INFO; i++) {
                        avgDeltaTimes += deltaTimes[i];
                        avgUpdateTimes += updateTimes[i];
                        avgDrawTimes += drawTimes[i];
                        avgBuffSwapTimes += buffSwapTimes[i];
                        avgOtherEventsTimes += otherEventsTimes[i];
                }

                avgDeltaTimes /= FRAMES_TIME_INFO;
                avgUpdateTimes /= FRAMES_TIME_INFO;
                avgDrawTimes /= FRAMES_TIME_INFO;
                avgBuffSwapTimes /= FRAMES_TIME_INFO;
                avgOtherEventsTimes /= FRAMES_TIME_INFO;

                double percUpdateTime =
                        avgUpdateTimes / avgDeltaTimes * 100.0;
                double percDrawTime =
                        avgDrawTimes / avgDeltaTimes * 100.0;
                double percBuffSwapTime =
                        avgBuffSwapTimes / avgDeltaTimes * 100.0;
                double percOtherEventsTime =
                        avgOtherEventsTimes / avgDeltaTimes * 100.0;

                double fps = 1.0 / avgDeltaTimes;

                fprintf(stderr,
                        "Average timing informatio of the last %u frames:\n",
                        FRAMES_TIME_INFO);
                fprintf(stderr, "\tTotal frame time:  %f (%f FPS)\n",
                        avgDeltaTimes, fps);
                fprintf(stderr, "\tUpdate time:       %f (%f%%)\n",
                        avgUpdateTimes, percUpdateTime);
                fprintf(stderr, "\tDraw time:         %f (%f%%)\n",
                        avgDrawTimes, percDrawTime);
                fprintf(stderr, "\tBuffer swap time:  %f (%f%%)\n",
                        avgBuffSwapTimes, percBuffSwapTime);
                fprintf(stderr, "\tOther events time: %f (%f%%)\n",
                        avgOtherEventsTimes, percOtherEventsTime);
        }
}

#endif

void window_run(void) {
        while (!glfwWindowShouldClose(window)) {
                // Update deltatime
                timeDelta = (const float)glfwGetTime();
                glfwSetTime(0);

                // Update game state
                eventFire_update();
                
#ifndef NDEBUG
                double updateTime = glfwGetTime();
#endif

                // Clear screen
                glClearColor(window_clearColor.x, window_clearColor.y,
                             window_clearColor.z, window_clearColor.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Draw stuff
                eventFire_draw();
                
#ifndef NDEBUG
                double drawTime = glfwGetTime();
#endif

                glfwSwapBuffers(window);
                
#ifndef NDEBUG
                double buffSwapTime = glfwGetTime();
#endif

                // Process inputs
                glfwPollEvents();
                eventFire_keyboardInput();
                
                eventBroker_runAsyncEvents();
                
#ifndef NDEBUG
                double otherEventsTime = glfwGetTime();
#endif
                
                
#ifndef NDEBUG
                updateTimingInformation(
                        (double)timeDelta, updateTime, drawTime,
                        buffSwapTime, otherEventsTime);
#endif
        }

        // Clean up
        eventFire_tearDown();
        eventBroker_shutdown();
        glfwTerminate();
}

float window_timeDelta(void) {
        return timeDelta;
}

bool window_keyPressed(const int key) {
        return glfwGetKey(window, key) == GLFW_PRESS;
}

void window_updateTitle(const char *const title) {
        if (title == NULL) {
                glfwSetWindowTitle(window, "");
        } else {
                glfwSetWindowTitle(window, title);
        }
}

void window_close(void) {
        glfwSetWindowShouldClose(window, 1);
}
