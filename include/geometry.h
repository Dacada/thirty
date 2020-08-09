#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <component.h>
#include <cglm/struct.h>
#include <glad/glad.h>

/*
 * This module encapsulates OpenGL geometry. Given the geometric data, a 3D
 * geometry is initialized. Its model matrix is also tracked. The draw method
 * uses the model matrix to draw the geometry. Tearing down a geometry frees
 * any data used in the struct geometry as well as by OpenGL. Geometries can be
 * initialized from arrays with all the 3D coordinates.
 */

struct vertex {
        vec3s vert;
        vec2s tex;
        vec3s norm;
        vec3s tang;
        vec3s binorm;
};

struct geometry {
        struct component base;
        GLuint vao, vbo, ibo;
        int nindices;
};

void geometry_initFromArray(struct geometry *geometry,
                            const char *name,
                            const struct vertex *vertices,
                            size_t nvertices,
                            const unsigned *indices,
                            size_t nindices)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((access (read_only, 3, 4)))
        __attribute__((access (read_only, 5, 6)))
        __attribute__((leaf))
        __attribute__((nonnull (1)));

void geometry_initSkybox(struct geometry *skybox, const char *name)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((leaf))
        __attribute__((nonnull (1)));

size_t geometry_initFromFile(struct geometry *geometry, FILE *f,
                             enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

void geometry_draw(const struct geometry *geometry)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void geometry_free(struct geometry *geometry)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#define GEOMETRY_MAXIMUM_SIZE sizeof(struct geometry)

#endif /* GEOMETRY_H */
