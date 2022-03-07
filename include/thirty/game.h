#ifndef GAME_H
#define GAME_H

#include <thirty/scene.h>
#include <enet/enet.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <nuklear/defs.h>
#include <nuklear/glfw.h>

/*
 * Implementation of an overarching "game" structure that keeps global data
 * about the game as well as setting up the graphical window. It also owns and
 * manages memory for the scenes.
 */

struct game {
        GLFWwindow *window;
        float timeDelta;
        vec4s clearColor;

        ENetHost *client;
        ENetPeer *server;

        struct uiData {
                struct nk_glfw glfw;
                struct nk_context *ctx;
        } uiData;

        bool inScene;
        size_t currentScene;
        struct growingArray scenes;
};

/*
 * Initialize the game. On top of initializing the game structure, this also
 * sets the working directory, initializes the event broker system, the
 * component collection system, networking, the window and the GL context.
 */
void game_init(struct game *game, int width, int height,
               size_t customEvents,
               size_t initalSceneCapacity)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

/*
 * Sets up the client part of an ENet connection and connects to the
 * server. Specify how many channels the connection shall use, the expected
 * incoming and outgoing bandwidth (or 0 for either to assume any amount), the
 * server address (which will be resolved), the server port (which will be
 * converted to an adequate format) and initial data to send to the server.
 */
void game_connect(struct game *game, size_t channels, unsigned bandwidth_in,
                  unsigned bandwidth_out, const char *address, unsigned short port,
                  unsigned initial_data)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_only, 5)))
        __attribute__((nonnull (1)));

/*
 * Notifies server cleanly of a disconnection.
 */
void game_disconnect(struct game *game, unsigned final_data)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Return a pointer to a newly allocated scene. It's completely uninitialized
 * except for the idx member.
 */
struct scene *game_createScene(struct game *game)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Return a pointer to the current scene that is being updated and
 * drawn. Returns NULL if there is no current scene.
 */
struct scene *game_getCurrentScene(const struct game *game)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Return a pointer to the scene corresponding to the given idx.
 */
struct scene *game_getSceneFromIdx(const struct game *game, size_t idx)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

/*
 * Set the game's current scene that will be updated and drawn.
 */
void game_setCurrentScene(struct game *game, size_t idx)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Unsets the game's current scene such that nothing will be updated or drawn.
 */
void game_unsetCurrentScene(struct game *game)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Set the title of the game window.
 */
void game_updateWindowTitle(struct game *game, const char *title)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull (1)));

/*
 * Set OpenGL's clear color.
 */
void game_setClearColor(struct game *game, vec4s color)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Get the time it took for the previous frame to complete.
 */
float game_timeDelta(const struct game *game)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Return whether the given GLFW key is pressed.
 */
bool game_keyPressed(const struct game *game, int key)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Return whether the given GLFW mouse button is pressed.
 */
bool game_mouseButtonPressed(const struct game *game, int button)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Set cursor position
 */
void game_setCursorPosition(const struct game *game, vec2s position)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Main loop of the game.
 */
void game_run(struct game *game)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Call to signal that the game loop should end.
 */
void game_shouldStop(struct game *game)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Free any resources used by the game as well as deinitializing everything
 * init initialized.
 */
void game_free(struct game *game)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
