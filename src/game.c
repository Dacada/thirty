#include <game.h>
#include <util.h>

#define FUNCTION_SIZE 256
#define DEFAULT_CLEARCOLOR {.x=0.2F, .y=0.3F, .z=0.3F, .w=1.0F}
#define STARTING_TIMEDELTA (1.0F/60.0F)

static void onFramebufferSizeChanged(void *registerArgs, void *fireArgs) {
        struct game *game = registerArgs;
        struct eventBrokerWindowResized *args = fireArgs;
        const int width = args->width;
        const int height = args->height;
        
        glViewport(0, 0, width, height);

        struct ui *ui = game_getCurrentUi(game);
        ui_resize(ui, width, height);
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

static void eventFire_update(const float timeDelta) {
        struct eventBrokerUpdate args = {
                .timeDelta = timeDelta,
        };
        eventBroker_fire(EVENT_BROKER_UPDATE, &args);
}

static void eventFire_draw(void) {
        eventBroker_fire(EVENT_BROKER_DRAW, NULL);
}

static void eventFire_tearDown(void) {
        eventBroker_fire(EVENT_BROKER_TEAR_DOWN, NULL);
}

static void doUpdateScene(const size_t sceneIdx,
                          struct growingArray *scenes, const float timeDelta) {
        struct scene *scene = growingArray_get(scenes, sceneIdx);
        scene_update(scene, timeDelta);
}

static void doUpdateUi(const size_t uiIdx,
                       struct growingArray *uis, const float timeDelta) {
        struct ui *ui = growingArray_get(uis, uiIdx);
        ui_update(ui, timeDelta);
}

static void doDrawScene(const size_t sceneIdx,
                        struct growingArray *scenes) {
        struct scene *scene = growingArray_get(scenes, sceneIdx);
        scene_draw(scene);

#ifndef NDEBUG
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
                fprintf(stderr, "OpenGL Error: %u", error);
        }
#endif
}

static void doDrawUi(const size_t uiIdx,
                     struct growingArray *uis) {
        struct ui *ui = growingArray_get(uis, uiIdx);
        ui_draw(ui);

#ifndef NDEBUG
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
                fprintf(stderr, "OpenGL Error: %u", error);
        }
#endif
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

void game_init(struct game *const game,
               const int width, const int height,
               const size_t initalSceneCapacity,
               const size_t initialUiCapacity) {
        eventBroker_startup();
        componentCollection_startup();
        physicalWorld_startup();
        ui_startup();

        glfwInit();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#else
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_FALSE);
#endif

        game->window = glfwCreateWindow(width, height, "", NULL, NULL);
        if (game->window == NULL) {
                glfwTerminate();
                die("Failed to create GLFW window.\n");
        }
        
        glfwMakeContextCurrent(game->window);

        if (!gladLoadGLLoader((const GLADloadproc)glfwGetProcAddress)) {
                glfwTerminate();
                die("Failed to initialize GLAD.\n");
        }

        glViewport(0, 0, width, height);
        glfwSetFramebufferSizeCallback(game->window, eventFire_windowResized);
        glfwSetKeyCallback(game->window, eventFire_keyboardEvent);
        glfwSetCursorPosCallback(game->window, eventFire_mousePosition);
        glfwSetScrollCallback(game->window, eventFire_mouseScroll);

        eventBroker_register(onFramebufferSizeChanged,
                             EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_WINDOW_RESIZED, game);
        
        glEnable(GL_DEPTH_TEST);
#ifdef NDEBUG
        glEnable(GL_CULL_FACE);
#else
        glDisable(GL_CULL_FACE);
#endif
        glfwSetInputMode(game->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        growingArray_init(&game->scenes,
                          sizeof(struct scene), initalSceneCapacity);
        growingArray_init(&game->uis,
                          sizeof(struct ui), initialUiCapacity);

        game->timeDelta = STARTING_TIMEDELTA;
        vec4s defaultClearColor = DEFAULT_CLEARCOLOR;
        game->clearColor = defaultClearColor;
}

struct scene *game_createScene(struct game *const game) {
        struct scene *scene = growingArray_append(&game->scenes);
        scene->idx = game->scenes.length - 1;
        return scene;
}

struct ui *game_createUi(struct game *const game) {
        struct ui *ui = growingArray_append(&game->uis);
        ui->idx = game->uis.length - 1;
        return ui;
}

struct scene *game_getCurrentScene(struct game *game) {
        return growingArray_get(&game->scenes, game->currentScene);
}

struct ui *game_getCurrentUi(struct game *game) {
        return growingArray_get(&game->uis, game->currentUi);
}

void game_setCurrentScene(struct game *const game, const size_t idx) {
        game->currentScene = idx;
}

void game_setCurrentUi(struct game *const game, const size_t idx) {
        game->currentUi = idx;
}

void game_updateWindowTitle(struct game *game, const char *title) {
        if (title == NULL) {
                glfwSetWindowTitle(game->window, "");
        } else {
                glfwSetWindowTitle(game->window, title);
        }
}

void game_setClearColor(struct game *game, vec4s color) {
        game->clearColor = color;
}

float game_timeDelta(const struct game *game) {
        return game->timeDelta;
}

bool game_keyPressed(const struct game *game, int key) {
        return glfwGetKey(game->window, key) == GLFW_PRESS;
}

void game_run(struct game *game) {
        while (!glfwWindowShouldClose(game->window)) {
                // Update deltatime
                game->timeDelta = (const float)glfwGetTime();
                glfwSetTime(0);

                // Update game state
                doUpdateScene(game->currentScene, &game->scenes,
                              game->timeDelta);
                doUpdateUi(game->currentUi, &game->uis,
                           game->timeDelta);
                eventFire_update(game->timeDelta);
                
#ifndef NDEBUG
                double updateTime = glfwGetTime();
#endif

                // Clear screen
                glClearColor(game->clearColor.x, game->clearColor.y,
                             game->clearColor.z, game->clearColor.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Draw stuff
                doDrawScene(game->currentScene, &game->scenes);
                doDrawUi(game->currentUi, &game->uis);
                eventFire_draw();
                
#ifndef NDEBUG
                double drawTime = glfwGetTime();
#endif

                glfwSwapBuffers(game->window);
                
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
                        (double)game->timeDelta, updateTime, drawTime,
                        buffSwapTime, otherEventsTime);
#endif
        }

        // Clean up
        eventFire_tearDown();
}

void game_shouldStop(struct game *game) {
        glfwSetWindowShouldClose(game->window, 1);
}

void game_free(struct game *const game) {
        growingArray_foreach_START(&game->scenes, struct scene *, scene)
                scene_free(scene);
        growingArray_foreach_END;
        growingArray_destroy(&game->scenes);
        
        growingArray_foreach_START(&game->uis, struct ui *, ui)
                ui_free(ui);
        growingArray_foreach_END;
        growingArray_destroy(&game->uis);

        ui_shutdown();
        physicalWorld_shutdown();
        componentCollection_shutdown();
        eventBroker_shutdown();
        glfwTerminate();
}
