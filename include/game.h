#ifndef GAME_H
#define GAME_H

#include <scene.h>
#include <eventBroker.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stddef.h>

/*
 * Implementation of an overarching "game" structure that keeps global data
 * about the game as well as setting up the graphical window. It also owns and
 * manages memory for the scenes.
 */

struct game {
        GLFWwindow *window;
        float timeDelta;
        vec4s clearColor;
        
        size_t currentScene;
        struct growingArray scenes;
};

/*
 * Initialize the game. On top of initializing the game structure, this also
 * initializes the event broker system and the component collection system. As
 * well as the window and GL context.
 */
void game_init(struct game *game, int width, int height,
               size_t initalSceneCapacity)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

// TODO
void game_initFromFile(struct game *game, const char *filename);

/*
 * Return a pointer to a newly allocated scene. It's completely uninitialized
 * except for the idx member.
 */
struct scene *game_createScene(struct game *game)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull))
        __attribute__((returns_nonnull));

struct scene *game_getCurrentScene(struct game *game)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Set the game's current scene that will be updated and drawn. Other scenes
 * will be ignored.
 */
void game_setCurrentScene(struct game *game, size_t idx)
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
