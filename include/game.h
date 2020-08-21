#ifndef GAME_H
#define GAME_H

#include <scene.h>
#include <eventBroker.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stddef.h>

struct game {
        GLFWwindow *window;
        float timeDelta;
        vec4s clearColor;
        
        size_t currentScene;
        struct growingArray scenes;
        
        eventBrokerCallback updateCallback;
        eventBrokerCallback drawCallback;
};

void game_init(struct game *game, int width, int height,
               size_t initalSceneCapacity)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

struct scene *game_createScene(struct game *game)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void game_setCurrentScene(struct game *game, size_t idx)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void game_updateWindowTitle(struct game *game, const char *title)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull (1)));

void game_setClearColor(struct game *game, vec4s color)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

float game_timeDelta(const struct game *game)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

bool game_keyPressed(const struct game *game, int key)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

void game_run(struct game *game)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void game_shouldStop(struct game *game)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void game_free(struct game *game)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
