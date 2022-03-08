#include <thirty/game.h>
#include <thirty/util.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wcast-qual"

#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#include <nuklear/defs.h>
#include <nuklear/glfw.h>

#define UI_MAX_VERTEX_BUFFER 512 * 1024
#define UI_MAX_ELEMENT_BUFFER 128 * 1024

#pragma GCC diagnostic pop

#define FUNCTION_SIZE 256
#define DEFAULT_CLEARCOLOR {.x=0.2F, .y=0.3F, .z=0.3F, .w=1.0F}
#define STARTING_TIMEDELTA (1.0F/60.0F)

static void onFramebufferSizeChanged(void *registerArgs, void *fireArgs) {
        (void)registerArgs;
        struct eventBrokerWindowResized *args = fireArgs;
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

static void eventFire_keyboardChar(GLFWwindow *const w, const unsigned int codepoint) {
        (void)w;
        struct eventBrokerKeyboardChar args = {
                .codepoint = codepoint,
        };
        nk_glfw3_char_callback(w, codepoint);
        eventBroker_fire(EVENT_BROKER_KEYBOARD_CHAR, &args);
}

static void eventFire_keyboardPoll(void) {
        eventBroker_fire(EVENT_BROKER_KEYBOARD_POLL, NULL);
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
        nk_gflw3_scroll_callback(w, xoff, yoff);
        eventBroker_fire(EVENT_BROKER_MOUSE_SCROLL, &args);
}

static void eventFire_mouseButton(GLFWwindow *const w,
                                  const int button, const int action, const int mods) {
        (void)w;
        struct eventBrokerMouseButton args = {
                .button = button,
                .action = action,
                .modifiers = mods,
        };
        nk_glfw3_mouse_button_callback(w, button, action, mods);
        eventBroker_fire(EVENT_BROKER_MOUSE_BUTTON, &args);
}

static void eventFire_mousePoll(void) {
        eventBroker_fire(EVENT_BROKER_MOUSE_POLL, NULL);
}

static void eventFire_update(const float timeDelta) {
        struct eventBrokerUpdate args = {
                .timeDelta = timeDelta,
        };
        eventBroker_fire(EVENT_BROKER_UPDATE, &args);
}

static void eventFire_updateUI(const struct game *const game, struct nk_context *const ctx) {
        int height, width;
        glfwGetWindowSize(game->window, &width, &height);
        
        struct eventBrokerUpdateUI args = {
                .ctx = ctx,
                .winHeight = height,
                .winWidth = width,
        };
        eventBroker_fire(EVENT_BROKER_UPDATE_UI, &args);
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

#ifndef NDEBUG
#define FRAMES_TIME_INFO 300

static void updateTimingInformation(double deltaTime, double networkingTime,
                                    double updateTime, double drawTime, double buffSwapTime,
                                    double otherEventsTime) {
        
        static double deltaTimes[FRAMES_TIME_INFO];
        static double networkingTimes[FRAMES_TIME_INFO];
        static double updateTimes[FRAMES_TIME_INFO];
        static double drawTimes[FRAMES_TIME_INFO];
        static double buffSwapTimes[FRAMES_TIME_INFO];
        static double otherEventsTimes[FRAMES_TIME_INFO];
        static unsigned current = 0;

        deltaTimes[current] = deltaTime;
        networkingTimes[current] = networkingTime;
        updateTimes[current] = updateTime;
        drawTimes[current] = drawTime - updateTime;
        buffSwapTimes[current] = buffSwapTime - drawTime;
        otherEventsTimes[current] = otherEventsTime - buffSwapTime;
        current++;

        if (current >= FRAMES_TIME_INFO) {
                current = 0;
                
                double avgDeltaTimes = 0;
                double avgNetworkingTimes = 0;
                double avgUpdateTimes = 0;
                double avgDrawTimes = 0;
                double avgBuffSwapTimes = 0;
                double avgOtherEventsTimes = 0;

                for (unsigned i=0; i<FRAMES_TIME_INFO; i++) {
                        avgDeltaTimes += deltaTimes[i];
                        avgNetworkingTimes += networkingTimes[i];
                        avgUpdateTimes += updateTimes[i];
                        avgDrawTimes += drawTimes[i];
                        avgBuffSwapTimes += buffSwapTimes[i];
                        avgOtherEventsTimes += otherEventsTimes[i];
                }

                avgDeltaTimes /= FRAMES_TIME_INFO;
                avgNetworkingTimes /= FRAMES_TIME_INFO;
                avgUpdateTimes /= FRAMES_TIME_INFO;
                avgDrawTimes /= FRAMES_TIME_INFO;
                avgBuffSwapTimes /= FRAMES_TIME_INFO;
                avgOtherEventsTimes /= FRAMES_TIME_INFO;

                double percNetworkingTime =
                        avgNetworkingTimes / avgDeltaTimes * 100.0;
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
                fprintf(stderr, "\tNetworking time:   %f (%f%%)\n",
                        avgNetworkingTimes, percNetworkingTime);
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

static void error_callback(int err, const char *const msg) {
        fprintf(stderr, "GLFW error %d: %s\n", err, msg);
}

static void setupGLContext(void) {
        glEnable(GL_DEPTH_TEST);
#ifdef NDEBUG
        glEnable(GL_CULL_FACE);
#else
        glDisable(GL_CULL_FACE);
#endif
        glEnable(GL_DEPTH_TEST);
}

void game_init(struct game *const game,
               const int width, const int height,
               const size_t customEvents,
               const size_t initalSceneCapacity) {
        set_cwd("../assets");

        if (enet_initialize() != 0) {
                die("Failed to initialize ENet.");
        }
        
        eventBroker_startup(customEvents);

        glfwSetErrorCallback(error_callback);
        if (!glfwInit()) {
                glfwTerminate();
                die("Failed to initialize GLFW.\n");
        }

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
        glfwSetCharCallback(game->window, eventFire_keyboardChar);
        glfwSetCursorPosCallback(game->window, eventFire_mousePosition);
        glfwSetScrollCallback(game->window, eventFire_mouseScroll);
        glfwSetMouseButtonCallback(game->window, eventFire_mouseButton);

        eventBroker_register(onFramebufferSizeChanged,
                             EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_WINDOW_RESIZED, NULL);
        
        setupGLContext();
        
        glfwSetInputMode(game->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        game->inScene = false;
        game->currentScene = 0;
        game->sceneMustChange = false;
        game->sceneToChangeTo = 0;
        growingArray_init(&game->scenes,
                          sizeof(struct scene), initalSceneCapacity);

        game->timeDelta = STARTING_TIMEDELTA;
        vec4s defaultClearColor = DEFAULT_CLEARCOLOR;
        game->clearColor = defaultClearColor;
        game->client = NULL;
        game->server = NULL;

        game->uiData.ctx = nk_glfw3_init(&game->uiData.glfw, game->window, NK_GLFW3_DEFAULT);
        {
                struct nk_font_atlas *atlas;
                nk_glfw3_font_stash_begin(&game->uiData.glfw, &atlas);
                nk_glfw3_font_stash_end(&game->uiData.glfw);
                nk_style_load_all_cursors(game->uiData.ctx, atlas->cursors);
        }
}

void game_connect(struct game *const game, const size_t channels,
                  const unsigned bandwidth_in, const unsigned bandwidth_out,
                  const char *const address, const unsigned short port,
                  const unsigned initial_data) {
        game->client = enet_host_create(NULL, 1, channels, bandwidth_in, bandwidth_out);
        if (game->client == NULL) {
                die("could not create host");
        }
        
        ENetAddress addr;
        enet_address_set_host(&addr, address);
        addr.port = port;
        
        game->server = enet_host_connect(game->client, &addr, channels, initial_data);
        if (game->server == NULL) {
                die("could not create server connection");
        }
}

void game_disconnect(struct game *const game, unsigned data) {
        if (game->server != NULL) {
                enet_peer_disconnect(game->server, data);
        }
}

struct scene *game_createScene(struct game *const game) {
        struct scene *scene = growingArray_append(&game->scenes);
        scene->idx = game->scenes.length - 1;
        return scene;
}

struct scene *game_getCurrentScene(const struct game *game) {
        if (game->inScene) {
                return game_getSceneFromIdx(game, game->currentScene);
        }
        return NULL;
}

struct scene *game_getSceneFromIdx(const struct game *game, size_t idx) {
        return growingArray_get(&game->scenes, idx);
}

static void doChangeScene(struct game *const game) {
        struct eventBrokerSceneChanged args = {
                .prevSceneIdx = game->currentScene,
        };

        if (!game->sceneMustChange && !game->sceneMustUnset) {
                return;
        }

        if (game->inScene) {
                scene_unload(game_getCurrentScene(game));
        }
        
        if (game->sceneMustChange) {
                scene_load(game_getSceneFromIdx(game, game->sceneToChangeTo));
                game->currentScene = game->sceneToChangeTo;
                game->inScene = true;
        } else if (game->sceneMustUnset) {
                game->inScene = false;
        } else {
                return;
        }
        
        game->sceneMustChange = false;
        game->sceneMustUnset = false;

        eventBroker_fire(EVENT_BROKER_SCENE_CHANGED, &args);
}

void game_setCurrentScene(struct game *const game, const size_t idx) {
        if (game->sceneMustUnset) {
                // we just called game_unsetCurrentScene, change to setting a scene
        } else if (game->sceneMustChange && game->sceneToChangeTo != idx) {
                // we just called this function but to a different scene
        } else if (game->inScene && game->currentScene != idx) {
                // normal call and we're not in that scene already
        } else if (!game->inScene) {
                // normal call and we're not in any scene
        } else {
                // in any other case, do nothing
                return;
        }

        game->sceneMustChange = true;
        game->sceneMustUnset = false;
        game->sceneToChangeTo = idx;
}

void game_unsetCurrentScene(struct game *const game) {
        if (game->sceneMustChange) {
                // we just called game_setCurrentScene but we're unsetting it instead
        } else if (game->inScene) {
                // normal call, we're in a scene right now
        } else {
                // in any other case, do nothing
                return;
        }

        game->sceneMustChange = false;
        game->sceneMustUnset = true;
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

bool game_mouseButtonPressed(const struct game *game, int button) {
        return glfwGetMouseButton(game->window, button) == GLFW_PRESS;
}

void game_setCursorPosition(const struct game *game, vec2s position) {
        glfwSetCursorPos(game->window, position.x, position.y);
}

void game_run(struct game *game) {
        while (!glfwWindowShouldClose(game->window)) {
                // Start process of changing scene, if necessary
                doChangeScene(game);
                
                // Update deltatime
                game->timeDelta = (const float)glfwGetTime();
                game->uiData.ctx->delta_time_seconds = game->timeDelta;
                glfwSetTime(0);

                // Process networking events
                static ENetEvent event;
                if (game->client != NULL) {
                        while (enet_host_service(game->client, &event, 0) > 0) {
                                enum eventBrokerEvent e;
                                switch (event.type) {
                                case ENET_EVENT_TYPE_CONNECT:
                                        e = EVENT_BROKER_NETWORK_CONNECTED;
                                        break;
                                case ENET_EVENT_TYPE_RECEIVE:
                                        e = EVENT_BROKER_NETWORK_RECV;
                                        break;
                                case ENET_EVENT_TYPE_DISCONNECT:
                                        e = EVENT_BROKER_NETWORK_DISCONNECTED;
                                        break;
                                case ENET_EVENT_TYPE_NONE:
                                default:
                                        e = EVENT_BROKER_EVENTS_TOTAL;
                                        break;
                                }
                                if (e != EVENT_BROKER_EVENTS_TOTAL) {
                                        eventBroker_fire(e, &event);
                                }
                        }
                }
#ifndef NDEBUG
                double networkingTime = glfwGetTime();
#endif

                // Process inputs
                glfwPollEvents();
                eventFire_keyboardPoll();
                eventFire_mousePoll();
                nk_glfw3_new_frame(&game->uiData.glfw);

                // Update game state
                if (game->inScene) {
                        doUpdateScene(game->currentScene, &game->scenes,
                                      game->timeDelta);
                }
                eventFire_updateUI(game, game->uiData.ctx);
                eventFire_update(game->timeDelta);
                
#ifndef NDEBUG
                double updateTime = glfwGetTime();
#endif

                // Clear screen
                glClearColor(game->clearColor.x, game->clearColor.y,
                             game->clearColor.z, game->clearColor.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Draw stuff
                setupGLContext();
                if (game->inScene) {
                        doDrawScene(game->currentScene, &game->scenes);
                }
                eventFire_draw();
                nk_glfw3_render(&game->uiData.glfw, NK_ANTI_ALIASING_ON, UI_MAX_VERTEX_BUFFER, UI_MAX_ELEMENT_BUFFER);
                
#ifndef NDEBUG
                double drawTime = glfwGetTime();
#endif

                glfwSwapBuffers(game->window);
                
#ifndef NDEBUG
                double buffSwapTime = glfwGetTime();
#endif

                // Process low priority events
                eventBroker_runAsyncEvents();
                
#ifndef NDEBUG
                double otherEventsTime = glfwGetTime();
#endif
                
                
#ifndef NDEBUG
                updateTimingInformation(
                        (double)game->timeDelta, networkingTime, updateTime, drawTime,
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
        if (game->client != NULL) {
                ENetEvent event;
                bool disconnected = false;
                while (enet_host_service(game->client, &event, 1000) > 0) {
                        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                                enet_packet_destroy(event.packet);
                        } else if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
                                disconnected = true;
                                break;
                        }
                }
                if (!disconnected) {
                        enet_peer_reset(game->server);
                }
                enet_host_destroy(game->client);
        }
        
        growingArray_foreach_START(&game->scenes, struct scene *, scene)
                scene_free(scene);
        growingArray_foreach_END;
        growingArray_destroy(&game->scenes);
        
        eventBroker_shutdown();
        enet_deinitialize();
        nk_glfw3_shutdown(&game->uiData.glfw);
        glfwTerminate();
}
