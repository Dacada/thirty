#include <geometry.h>
#include <vertex.h>
#include <component.h>
#include <util.h>
#include <cglm/struct.h>
#include <glad/glad.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>

#define VERTEX_ATTRIB 0
#define TEXCOORD_ATTRIB 1
#define NORMAL_ATTRIB 2
#define TANGENT_ATTRIB 3
#define BINORMAL_ATTRIB 4
#define BONEIDX_ATTRIB 5
#define BONEWGHT_ATTRIB 6

__attribute__((access (write_only, 1)))
__attribute__((nonnull))
static void geometry_init(struct geometry *const geometry) {
        geometry->vao = 0;
        geometry->vbo = 0;
        geometry->ibo = 0;
}

void geometry_initFromArray(struct geometry *const geometry,
                            const char *const name,
                            const struct vertex *const vertices,
                            const size_t nvertices,
                            const unsigned *const indices,
                            const size_t nindices) {
        if (nindices > INT_MAX) {
                bail("Cannot draw geometry with more than %d "
                     "indices (attempted geometry has %lu indices)\n",
                     INT_MAX, nindices);
        }

        component_init((struct component *)geometry, name);
        geometry_init(geometry);
        
        glGenVertexArrays(1, &geometry->vao);
        glBindVertexArray(geometry->vao);

        glGenBuffers(1, &geometry->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, geometry->vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)(nvertices * sizeof(*vertices)),
                     vertices, GL_STATIC_DRAW);

#define VERTEX_ATTRIB_PTR(idx, name)                                    \
        glEnableVertexAttribArray(idx);                                 \
        glVertexAttribPointer(                                          \
                idx, sizeof(vertices->name)/sizeof(float),              \
                GL_FLOAT, GL_FALSE, sizeof(struct vertex),              \
                (const void *const)offsetof(struct vertex, name));

        VERTEX_ATTRIB_PTR(VERTEX_ATTRIB, vert);
        VERTEX_ATTRIB_PTR(TEXCOORD_ATTRIB, tex);
        VERTEX_ATTRIB_PTR(NORMAL_ATTRIB, norm);
        VERTEX_ATTRIB_PTR(TANGENT_ATTRIB, tang);
        VERTEX_ATTRIB_PTR(BINORMAL_ATTRIB, binorm);
        VERTEX_ATTRIB_PTR(BONEIDX_ATTRIB, bones);
        VERTEX_ATTRIB_PTR(BONEWGHT_ATTRIB, weights);

#undef VERTEX_ATTRIB_PTR
        
        glGenBuffers(1, &geometry->ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->ibo);
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)(nindices * sizeof(*indices)),
                     indices, GL_STATIC_DRAW);
        
        glBindVertexArray(0);
        geometry->nindices = (const int)nindices;
}

void geometry_initCube(struct geometry *geo, const char *const name) {
        static const struct vertex vertices[] = {
                {.vert.x =  1, .vert.y =  1, .vert.z =  1,
                 .bones={{0,0,0}}, .weights={{0,0,0}}},
                {.vert.x =  1, .vert.y =  1, .vert.z = -1,
                 .bones={{0,0,0}}, .weights={{0,0,0}}},
                {.vert.x =  1, .vert.y = -1, .vert.z =  1,
                 .bones={{0,0,0}}, .weights={{0,0,0}}},
                {.vert.x =  1, .vert.y = -1, .vert.z = -1,
                 .bones={{0,0,0}}, .weights={{0,0,0}}},
                {.vert.x = -1, .vert.y =  1, .vert.z =  1,
                 .bones={{0,0,0}}, .weights={{0,0,0}}},
                {.vert.x = -1, .vert.y =  1, .vert.z = -1,
                 .bones={{0,0,0}}, .weights={{0,0,0}}},
                {.vert.x = -1, .vert.y = -1, .vert.z =  1,
                 .bones={{0,0,0}}, .weights={{0,0,0}}},
                {.vert.x = -1, .vert.y = -1, .vert.z = -1,
                 .bones={{0,0,0}}, .weights={{0,0,0}}},
        };
        static const size_t nvertices = sizeof(vertices) / sizeof(*vertices);
        
        static const unsigned indices[] = {
                0, 2, 6,  0, 6, 4,
                4, 6, 7,  4, 7, 5,
                5, 7, 3,  5, 3, 1,
                1, 3, 2,  1, 2, 0,
                1, 0, 4,  1, 4, 5,
                2, 3, 7,  2, 7, 6,
        };
        static const size_t nindices = sizeof(indices) / sizeof(*indices);
        
        geometry_initFromArray(geo, name,
                               vertices, nvertices, indices, nindices);
}

size_t geometry_initFromFile(struct geometry *const geometry, FILE *const f,
                             const enum componentType type) {
        (void)type;
        
        struct {
                uint32_t vertlen;
                uint32_t indlen;
        } header;

        char *name = strfile(f);

        sfread(&header.vertlen, sizeof(header.vertlen), 1, f);
        sfread(&header.indlen, sizeof(header.indlen), 1, f);
        
        struct vertex *const vertices =
                smallocarray(header.vertlen, sizeof(*vertices));
        unsigned *const indices =
                smallocarray(header.indlen, sizeof(*indices));

        sfread(vertices, sizeof(*vertices), header.vertlen, f);
        sfread(indices, sizeof(*indices), header.indlen, f);
        
        geometry_initFromArray(geometry, name,
                               vertices, header.vertlen,
                               indices, header.indlen);

        free(name);
        free(vertices);
        free(indices);
        return sizeof(struct geometry);
}

void geometry_draw(const struct geometry *const geometry) {
        glBindVertexArray(geometry->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->ibo);
        glDrawElements(GL_TRIANGLES, geometry->nindices, GL_UNSIGNED_INT, 0);
}

void geometry_free(struct geometry *const geometry) {
        component_free((struct component*)geometry);
        
        glDeleteBuffers(1, &geometry->vbo);
        glDeleteBuffers(1, &geometry->ibo);
        glDeleteVertexArrays(1, &geometry->vao);

        // Not necessary since deleted objects are ignored by glDelete*, but
        // more expressive. (zero is also ignored)
        geometry->vbo = 0;
        geometry->ibo = 0;
        geometry->vao = 0;
}
