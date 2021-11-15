#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <thirty/component.h>
#include <thirty/vertex.h>
#include <glad/glad.h>

/*
 * This component encapsulates OpenGL geometry, a mesh. Only objects with a
 * geometry component will be drawn.
 */

struct geometry {
        struct component base;
        GLuint vao, vbo, ibo;
        int nindices;
};

/*
 * Initialize a geometry from an array of indices and vertices. Them and the
 * name can be freed after initialization.
 */
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
        __attribute__((nonnull));

/*
 * Initializes a geometry to be a 1x1x1 cube. 
 */
void geometry_initCube(struct geometry *geo, const char *name)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Initializes a geometry to be a 1x1x1 cube with faces facing inwards.
 */
void geometry_initSkyboxCube(struct geometry *geo, const char *name)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Initializes a geometry to be a radius 1 icosphere with as many subdivisions
 * as given.
 */
void geometry_initIcosphere(struct geometry *geo, const char *name,
                            unsigned subdivisions)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Initializes a geometry to be a NxM plane on XY.
 */
void geometry_initPlane(struct geometry *geo, const char *name)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Initialize a geometry from a BOGLE file positioned at the correct offset.
 */
size_t geometry_initFromFile(struct geometry *geometry, FILE *f,
                             enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

/*
 * Draw the geometry using OpenGL, using whatever shader is set.
 */
void geometry_draw(const struct geometry *geometry)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Free all resources used by the geometry, deinitializing it.
 */
void geometry_free(struct geometry *geometry)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

#define GEOMETRY_MAXIMUM_SIZE sizeof(struct geometry)

#endif /* GEOMETRY_H */
