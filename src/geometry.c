#include <thirty/geometry.h>
#include <thirty/util.h>

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
        assert(nindices <= INT_MAX);
        assert(geometry->base.type == COMPONENT_GEOMETRY);

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
                {.vert.x   =-1.0000F,.vert.y   = 1.0000F,.vert.z   =1.0000F,
                 .tex.x    = 0.8750F,.tex.y    = 0.5000F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   =1.0000F,
                 .tang.x   =-1.0000F,.tang.y   = 0.0000F,.tang.z   =0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z =0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  =0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z=0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   = 1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.3750F,.tex.y    = 0.2500F,
                 .norm.x   =-1.0000F,.norm.y   = 0.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 0.0000F,.binorm.y = 1.0000F,.binorm.z =-0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   = 1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.6250F,.tex.y    = 0.2500F,
                 .norm.x   = 0.0000F,.norm.y   = 1.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 1.0000F,.binorm.y = 0.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   =-1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.3750F,.tex.y    = 0.7500F,
                 .norm.x   = 1.0000F,.norm.y   = 0.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   = 1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.6250F,.tex.y    = 0.5000F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   = 1.0000F,
                 .tang.x   =-1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   =-1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.3750F,.tex.y    = 0.7500F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   =-1.0000F,
                 .tang.x   = 1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   = 1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.3750F,.tex.y    = 0.5000F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   =-1.0000F,
                 .tang.x   = 1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   =-1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.3750F,.tex.y    = 1.0000F,
                 .norm.x   = 0.0000F,.norm.y   =-1.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x =-1.0000F,.binorm.y = 0.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   =-1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.1250F,.tex.y    = 0.7500F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   =-1.0000F,
                 .tang.x   = 1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   =-1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.6250F,.tex.y    = 0.7500F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   = 1.0000F,
                 .tang.x   =-1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   = 1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.3750F,.tex.y    = 0.5000F,
                 .norm.x   = 1.0000F,.norm.y   = 0.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   = 1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.6250F,.tex.y    = 0.5000F,
                 .norm.x   = 0.0000F,.norm.y   = 1.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 1.0000F,.binorm.y = 0.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   =-1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.6250F,.tex.y    = 0.7500F,
                 .norm.x   = 1.0000F,.norm.y   = 0.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   =-1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.8750F,.tex.y    = 0.7500F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   = 1.0000F,
                 .tang.x   =-1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   =-1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.6250F,.tex.y    = 1.0000F,
                 .norm.x   = 0.0000F,.norm.y   =-1.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x =-1.0000F,.binorm.y = 0.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   =-1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.3750F,.tex.y    = 0.7500F,
                 .norm.x   = 0.0000F,.norm.y   =-1.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x =-1.0000F,.binorm.y = 0.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   = 1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.1250F,.tex.y    = 0.5000F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   =-1.0000F,
                 .tang.x   = 1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   = 1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.3750F,.tex.y    = 0.2500F,
                 .norm.x   = 0.0000F,.norm.y   = 1.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 1.0000F,.binorm.y = 0.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   = 1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.6250F,.tex.y    = 0.2500F,
                 .norm.x   =-1.0000F,.norm.y   = 0.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 0.0000F,.binorm.y = 1.0000F,.binorm.z =-0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   =-1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.6250F,.tex.y    = 0.7500F,
                 .norm.x   = 0.0000F,.norm.y   =-1.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x =-1.0000F,.binorm.y = 0.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   = 1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.6250F,.tex.y    = 0.5000F,
                 .norm.x   = 1.0000F,.norm.y   = 0.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   = 1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.3750F,.tex.y    = 0.5000F,
                 .norm.x   = 0.0000F,.norm.y   = 1.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 1.0000F,.binorm.y = 0.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   =-1.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.6250F,.tex.y    = 0.0000F,
                 .norm.x   =-1.0000F,.norm.y   = 0.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 0.0000F,.binorm.y = 1.0000F,.binorm.z =-0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   =-1.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.3750F,.tex.y    = 0.0000F,
                 .norm.x   =-1.0000F,.norm.y   = 0.0000F,.norm.z   = 0.0000F,
                 .tang.x   = 0.0000F,.tang.y   = 0.0000F,.tang.z   = 1.0000F,
                 .binorm.x = 0.0000F,.binorm.y = 1.0000F,.binorm.z =-0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
        };
        
        static const unsigned indices[] = {
                0, 9, 4,
                0, 13, 9,
                19, 7, 15,
                19, 14, 7,
                22, 1, 23,
                22, 18, 1,
                6, 8, 16,
                6, 5, 8,
                20, 3, 10,
                20, 12, 3,
                2, 21, 17,
                2, 11, 21
        };
        
        size_t nvertices = sizeof(vertices) / sizeof(*vertices);
        size_t nindices = sizeof(indices) / sizeof(*indices);
        
        geometry_initFromArray(geo, name,
                               vertices, nvertices, indices, nindices);
}

void geometry_initIcosphere(struct geometry *const geo,
                            const char *const name,
                            const unsigned subdivisions) {
        static const struct vertex vertices[] = {
                {.vert.x   =-0.2764F,.vert.y   =-0.8506F,.vert.z   =-0.4472F,
                 .tex.x    = 0.9091F,.tex.y    = 0.1575F,
                 .norm.x   =-0.3035F,.norm.y   =-0.9342F,.norm.z   = 0.1876F,
                 .tang.x   =-0.9511F,.tang.y   = 0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0580F,.binorm.y = 0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.7236F,.vert.y   =-0.5257F,.vert.z   = 0.4472F,
                 .tex.x    = 0.0909F,.tex.y    = 0.3149F,
                 .norm.x   =-0.7946F,.norm.y   =-0.5774F,.norm.z   =-0.1876F,
                 .tang.x   =-0.5878F,.tang.y   = 0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.1518F,.binorm.y =-0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.2764F,.vert.y   = 0.8506F,.vert.z   = 0.4472F,
                 .tex.x    = 0.4545F,.tex.y    = 0.3149F,
                 .norm.x   =-0.1876F,.norm.y   = 0.5774F,.norm.z   = 0.7947F,
                 .tang.x   = 0.9511F,.tang.y   = 0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.2456F,.binorm.y =-0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.8944F,.vert.y   = 0.0000F,.vert.z   = 0.4472F,
                 .tex.x    = 0.6364F,.tex.y    = 0.3149F,
                 .norm.x   = 0.4911F,.norm.y   =-0.3568F,.norm.z   = 0.7947F,
                 .tang.x   =-0.5878F,.tang.y   =-0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.6429F,.binorm.y = 0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.8944F,.vert.y   = 0.0000F,.vert.z   =-0.4472F,
                 .tex.x    = 0.1818F,.tex.y    = 0.1575F,
                 .norm.x   =-0.4911F,.norm.y   = 0.3568F,.norm.z   =-0.7947F,
                 .tang.x   = 0.5878F,.tang.y   = 0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.6429F,.binorm.y = 0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.7236F,.vert.y   =-0.5257F,.vert.z   = 0.4472F,
                 .tex.x    = 1.0000F,.tex.y    = 0.3149F,
                 .norm.x   =-0.3035F,.norm.y   =-0.9342F,.norm.z   = 0.1876F,
                 .tang.x   =-0.9511F,.tang.y   = 0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0580F,.binorm.y = 0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.7236F,.vert.y   = 0.5257F,.vert.z   = 0.4472F,
                 .tex.x    = 0.2727F,.tex.y    = 0.3149F,
                 .norm.x   =-0.1876F,.norm.y   = 0.5774F,.norm.z   = 0.7947F,
                 .tang.x   = 0.9511F,.tang.y   = 0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.2456F,.binorm.y =-0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.7236F,.vert.y   = 0.5257F,.vert.z   =-0.4472F,
                 .tex.x    = 0.5455F,.tex.y    = 0.1575F,
                 .norm.x   = 0.9822F,.norm.y   = 0.0000F,.norm.z   =-0.1876F,
                 .tang.x   = 0.0000F,.tang.y   =-1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.1876F,.binorm.y = 0.0000F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.8944F,.vert.y   = 0.0000F,.vert.z   =-0.4472F,
                 .tex.x    = 0.1818F,.tex.y    = 0.1575F,
                 .norm.x   =-0.7946F,.norm.y   = 0.5774F,.norm.z   =-0.1876F,
                 .tang.x   = 0.5878F,.tang.y   = 0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.1518F,.binorm.y = 0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.2764F,.vert.y   = 0.8506F,.vert.z   =-0.4472F,
                 .tex.x    = 0.3636F,.tex.y    = 0.1575F,
                 .norm.x   =-0.7946F,.norm.y   = 0.5774F,.norm.z   =-0.1876F,
                 .tang.x   = 0.5878F,.tang.y   = 0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.1518F,.binorm.y = 0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.2764F,.vert.y   =-0.8506F,.vert.z   =-0.4472F,
                 .tex.x    = 0.9091F,.tex.y    = 0.1575F,
                 .norm.x   = 0.1876F,.norm.y   =-0.5774F,.norm.z   =-0.7947F,
                 .tang.x   =-0.9511F,.tang.y   =-0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.2456F,.binorm.y =-0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.2764F,.vert.y   =-0.8506F,.vert.z   = 0.4472F,
                 .tex.x    = 0.8182F,.tex.y    = 0.3149F,
                 .norm.x   = 0.4911F,.norm.y   =-0.3568F,.norm.z   = 0.7947F,
                 .tang.x   =-0.5878F,.tang.y   =-0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.6429F,.binorm.y = 0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.7236F,.vert.y   =-0.5257F,.vert.z   =-0.4472F,
                 .tex.x    = 0.7273F,.tex.y    = 0.1575F,
                 .norm.x   = 0.6071F,.norm.y   = 0.0000F,.norm.z   =-0.7947F,
                 .tang.x   = 0.0000F,.tang.y   =-1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.7947F,.binorm.y = 0.0000F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.2764F,.vert.y   = 0.8506F,.vert.z   =-0.4472F,
                 .tex.x    = 0.3636F,.tex.y    = 0.1575F,
                 .norm.x   =-0.3035F,.norm.y   = 0.9342F,.norm.z   = 0.1876F,
                 .tang.x   = 0.9511F,.tang.y   = 0.3090F,.tang.z   =-0.0000F,
                 .binorm.x = 0.0580F,.binorm.y =-0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.2764F,.vert.y   = 0.8506F,.vert.z   = 0.4472F,
                 .tex.x    = 0.4545F,.tex.y    = 0.3149F,
                 .norm.x   = 0.3035F,.norm.y   = 0.9342F,.norm.z   =-0.1876F,
                 .tang.x   = 0.9511F,.tang.y   =-0.3090F,.tang.z   =-0.0000F,
                 .binorm.x = 0.0580F,.binorm.y = 0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.7236F,.vert.y   =-0.5257F,.vert.z   =-0.4472F,
                 .tex.x    = 0.7273F,.tex.y    = 0.1575F,
                 .norm.x   = 0.9822F,.norm.y   = 0.0000F,.norm.z   =-0.1876F,
                 .tang.x   = 0.0000F,.tang.y   =-1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.1876F,.binorm.y = 0.0000F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.7236F,.vert.y   =-0.5257F,.vert.z   =-0.4472F,
                 .tex.x    = 0.7273F,.tex.y    = 0.1575F,
                 .norm.x   = 0.7946F,.norm.y   =-0.5774F,.norm.z   = 0.1876F,
                 .tang.x   =-0.5878F,.tang.y   =-0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.1518F,.binorm.y = 0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.2764F,.vert.y   =-0.8506F,.vert.z   = 0.4472F,
                 .tex.x    = 0.8182F,.tex.y    = 0.3149F,
                 .norm.x   = 0.3035F,.norm.y   =-0.9342F,.norm.z   =-0.1876F,
                 .tang.x   =-0.9511F,.tang.y   =-0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0580F,.binorm.y =-0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.2764F,.vert.y   = 0.8506F,.vert.z   = 0.4472F,
                 .tex.x    = 0.4545F,.tex.y    = 0.3149F,
                 .norm.x   =-0.3035F,.norm.y   = 0.9342F,.norm.z   = 0.1876F,
                 .tang.x   = 0.9511F,.tang.y   = 0.3090F,.tang.z   =-0.0000F,
                 .binorm.x = 0.0580F,.binorm.y =-0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.0000F,.vert.y   = 0.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.7273F,.tex.y    = 0.4724F,
                 .norm.x   = 0.4911F,.norm.y   =-0.3568F,.norm.z   = 0.7947F,
                 .tang.x   =-0.5878F,.tang.y   =-0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.6429F,.binorm.y = 0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.2764F,.vert.y   =-0.8506F,.vert.z   = 0.4472F,
                 .tex.x    = 0.8182F,.tex.y    = 0.3149F,
                 .norm.x   = 0.7946F,.norm.y   =-0.5774F,.norm.z   = 0.1876F,
                 .tang.x   =-0.5878F,.tang.y   =-0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.1518F,.binorm.y = 0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.0000F,.vert.y   = 0.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.9091F,.tex.y    = 0.4724F,
                 .norm.x   =-0.1876F,.norm.y   =-0.5774F,.norm.z   = 0.7947F,
                 .tang.x   =-0.9511F,.tang.y   = 0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.2456F,.binorm.y = 0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.7236F,.vert.y   = 0.5257F,.vert.z   = 0.4472F,
                 .tex.x    = 0.2727F,.tex.y    = 0.3149F,
                 .norm.x   =-0.6071F,.norm.y   = 0.0000F,.norm.z   = 0.7947F,
                 .tang.x   = 0.0000F,.tang.y   = 1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.7947F,.binorm.y =-0.0000F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.7236F,.vert.y   =-0.5257F,.vert.z   = 0.4472F,
                 .tex.x    = 0.0909F,.tex.y    = 0.3149F,
                 .norm.x   =-0.9822F,.norm.y   = 0.0000F,.norm.z   = 0.1876F,
                 .tang.x   = 0.0000F,.tang.y   = 1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.1876F,.binorm.y =-0.0000F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.2764F,.vert.y   = 0.8506F,.vert.z   =-0.4472F,
                 .tex.x    = 0.3636F,.tex.y    = 0.1575F,
                 .norm.x   =-0.4911F,.norm.y   = 0.3568F,.norm.z   =-0.7947F,
                 .tang.x   = 0.5878F,.tang.y   = 0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.6429F,.binorm.y = 0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.0000F,.vert.y   = 0.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.1818F,.tex.y    = 0.4724F,
                 .norm.x   =-0.6071F,.norm.y   = 0.0000F,.norm.z   = 0.7947F,
                 .tang.x   = 0.0000F,.tang.y   = 1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.7947F,.binorm.y =-0.0000F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.8944F,.vert.y   = 0.0000F,.vert.z   = 0.4472F,
                 .tex.x    = 0.6364F,.tex.y    = 0.3149F,
                 .norm.x   = 0.4911F,.norm.y   = 0.3568F,.norm.z   = 0.7947F,
                 .tang.x   = 0.5878F,.tang.y   =-0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.6429F,.binorm.y =-0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.8944F,.vert.y   = 0.0000F,.vert.z   =-0.4472F,
                 .tex.x    = 0.1818F,.tex.y    = 0.1575F,
                 .norm.x   =-0.7946F,.norm.y   =-0.5774F,.norm.z   =-0.1876F,
                 .tang.x   =-0.5878F,.tang.y   = 0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.1518F,.binorm.y =-0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.0000F,.vert.y   = 0.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.3636F,.tex.y    = 0.4724F,
                 .norm.x   =-0.1876F,.norm.y   = 0.5774F,.norm.z   = 0.7947F,
                 .tang.x   = 0.9511F,.tang.y   = 0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.2456F,.binorm.y =-0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.0000F,.vert.y   = 0.0000F,.vert.z   = 1.0000F,
                 .tex.x    = 0.5455F,.tex.y    = 0.4724F,
                 .norm.x   = 0.4911F,.norm.y   = 0.3568F,.norm.z   = 0.7947F,
                 .tang.x   = 0.5878F,.tang.y   =-0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.6429F,.binorm.y =-0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.0000F,.vert.y   = 0.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.6364F,.tex.y    = 0.0000F,
                 .norm.x   = 0.6071F,.norm.y   = 0.0000F,.norm.z   =-0.7947F,
                 .tang.x   = 0.0000F,.tang.y   =-1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.7947F,.binorm.y = 0.0000F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.7236F,.vert.y   =-0.5257F,.vert.z   = 0.4472F,
                 .tex.x    = 0.0909F,.tex.y    = 0.3149F,
                 .norm.x   =-0.6071F,.norm.y   = 0.0000F,.norm.z   = 0.7947F,
                 .tang.x   = 0.0000F,.tang.y   = 1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.7947F,.binorm.y =-0.0000F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.2764F,.vert.y   =-0.8506F,.vert.z   =-0.4472F,
                 .tex.x    = 0.0000F,.tex.y    = 0.1575F,
                 .norm.x   =-0.4911F,.norm.y   =-0.3568F,.norm.z   =-0.7947F,
                 .tang.x   =-0.5878F,.tang.y   = 0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.6429F,.binorm.y =-0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.7236F,.vert.y   = 0.5257F,.vert.z   =-0.4472F,
                 .tex.x    = 0.5455F,.tex.y    = 0.1575F,
                 .norm.x   = 0.3035F,.norm.y   = 0.9342F,.norm.z   =-0.1876F,
                 .tang.x   = 0.9511F,.tang.y   =-0.3090F,.tang.z   =-0.0000F,
                 .binorm.x = 0.0580F,.binorm.y = 0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.8944F,.vert.y   = 0.0000F,.vert.z   = 0.4472F,
                 .tex.x    = 0.6364F,.tex.y    = 0.3149F,
                 .norm.x   = 0.9822F,.norm.y   = 0.0000F,.norm.z   =-0.1876F,
                 .tang.x   = 0.0000F,.tang.y   =-1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.1876F,.binorm.y = 0.0000F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.7236F,.vert.y   = 0.5257F,.vert.z   = 0.4472F,
                 .tex.x    = 0.2727F,.tex.y    = 0.3149F,
                 .norm.x   =-0.7946F,.norm.y   = 0.5774F,.norm.z   =-0.1876F,
                 .tang.x   = 0.5878F,.tang.y   = 0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.1518F,.binorm.y = 0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.8944F,.vert.y   = 0.0000F,.vert.z   =-0.4472F,
                 .tex.x    = 0.1818F,.tex.y    = 0.1575F,
                 .norm.x   =-0.9822F,.norm.y   = 0.0000F,.norm.z   = 0.1876F,
                 .tang.x   = 0.0000F,.tang.y   = 1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.1876F,.binorm.y =-0.0000F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.7236F,.vert.y   = 0.5257F,.vert.z   =-0.4472F,
                 .tex.x    = 0.5455F,.tex.y    = 0.1575F,
                 .norm.x   = 0.7946F,.norm.y   = 0.5774F,.norm.z   = 0.1876F,
                 .tang.x   = 0.5878F,.tang.y   =-0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.1518F,.binorm.y =-0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.2764F,.vert.y   =-0.8506F,.vert.z   =-0.4472F,
                 .tex.x    = 0.0000F,.tex.y    = 0.1575F,
                 .norm.x   =-0.7946F,.norm.y   =-0.5774F,.norm.z   =-0.1876F,
                 .tang.x   =-0.5878F,.tang.y   = 0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.1518F,.binorm.y =-0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.7236F,.vert.y   = 0.5257F,.vert.z   = 0.4472F,
                 .tex.x    = 0.2727F,.tex.y    = 0.3149F,
                 .norm.x   =-0.9822F,.norm.y   = 0.0000F,.norm.z   = 0.1876F,
                 .tang.x   = 0.0000F,.tang.y   = 1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.1876F,.binorm.y =-0.0000F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.7236F,.vert.y   = 0.5257F,.vert.z   = 0.4472F,
                 .tex.x    = 0.2727F,.tex.y    = 0.3149F,
                 .norm.x   =-0.3035F,.norm.y   = 0.9342F,.norm.z   = 0.1876F,
                 .tang.x   = 0.9511F,.tang.y   = 0.3090F,.tang.z   =-0.0000F,
                 .binorm.x = 0.0580F,.binorm.y =-0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.2764F,.vert.y   = 0.8506F,.vert.z   = 0.4472F,
                 .tex.x    = 0.4545F,.tex.y    = 0.3149F,
                 .norm.x   = 0.7946F,.norm.y   = 0.5774F,.norm.z   = 0.1876F,
                 .tang.x   = 0.5878F,.tang.y   =-0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.1518F,.binorm.y =-0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.8944F,.vert.y   = 0.0000F,.vert.z   = 0.4472F,
                 .tex.x    = 0.6364F,.tex.y    = 0.3149F,
                 .norm.x   = 0.7946F,.norm.y   = 0.5774F,.norm.z   = 0.1876F,
                 .tang.x   = 0.5878F,.tang.y   =-0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.1518F,.binorm.y =-0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.8944F,.vert.y   = 0.0000F,.vert.z   =-0.4472F,
                 .tex.x    = 0.1818F,.tex.y    = 0.1575F,
                 .norm.x   =-0.4911F,.norm.y   =-0.3568F,.norm.z   =-0.7947F,
                 .tang.x   =-0.5878F,.tang.y   = 0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.6429F,.binorm.y =-0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.7236F,.vert.y   = 0.5257F,.vert.z   =-0.4472F,
                 .tex.x    = 0.5455F,.tex.y    = 0.1575F,
                 .norm.x   = 0.6071F,.norm.y   = 0.0000F,.norm.z   =-0.7947F,
                 .tang.x   = 0.0000F,.tang.y   =-1.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.7947F,.binorm.y = 0.0000F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.2764F,.vert.y   = 0.8506F,.vert.z   = 0.4472F,
                 .tex.x    = 0.4545F,.tex.y    = 0.3149F,
                 .norm.x   = 0.4911F,.norm.y   = 0.3568F,.norm.z   = 0.7947F,
                 .tang.x   = 0.5878F,.tang.y   =-0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.6429F,.binorm.y =-0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.2764F,.vert.y   = 0.8506F,.vert.z   =-0.4472F,
                 .tex.x    = 0.3636F,.tex.y    = 0.1575F,
                 .norm.x   = 0.1876F,.norm.y   = 0.5774F,.norm.z   =-0.7947F,
                 .tang.x   = 0.9511F,.tang.y   =-0.3090F,.tang.z   =-0.0000F,
                 .binorm.x = 0.2456F,.binorm.y = 0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.0000F,.vert.y   = 0.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.8182F,.tex.y    = 0.0000F,
                 .norm.x   = 0.1876F,.norm.y   =-0.5774F,.norm.z   =-0.7947F,
                 .tang.x   =-0.9511F,.tang.y   =-0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.2456F,.binorm.y =-0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.7236F,.vert.y   = 0.5257F,.vert.z   =-0.4472F,
                 .tex.x    = 0.5455F,.tex.y    = 0.1575F,
                 .norm.x   = 0.1876F,.norm.y   = 0.5774F,.norm.z   =-0.7947F,
                 .tang.x   = 0.9511F,.tang.y   =-0.3090F,.tang.z   =-0.0000F,
                 .binorm.x = 0.2456F,.binorm.y = 0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.2764F,.vert.y   =-0.8506F,.vert.z   = 0.4472F,
                 .tex.x    = 0.8182F,.tex.y    = 0.3149F,
                 .norm.x   =-0.3035F,.norm.y   =-0.9342F,.norm.z   = 0.1876F,
                 .tang.x   =-0.9511F,.tang.y   = 0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0580F,.binorm.y = 0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.7236F,.vert.y   =-0.5257F,.vert.z   =-0.4472F,
                 .tex.x    = 0.7273F,.tex.y    = 0.1575F,
                 .norm.x   = 0.1876F,.norm.y   =-0.5774F,.norm.z   =-0.7947F,
                 .tang.x   =-0.9511F,.tang.y   =-0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.2456F,.binorm.y =-0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.2764F,.vert.y   = 0.8506F,.vert.z   =-0.4472F,
                 .tex.x    = 0.3636F,.tex.y    = 0.1575F,
                 .norm.x   = 0.3035F,.norm.y   = 0.9342F,.norm.z   =-0.1876F,
                 .tang.x   = 0.9511F,.tang.y   =-0.3090F,.tang.z   =-0.0000F,
                 .binorm.x = 0.0580F,.binorm.y = 0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.0000F,.vert.y   = 0.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.2727F,.tex.y    = 0.0000F,
                 .norm.x   =-0.4911F,.norm.y   = 0.3568F,.norm.z   =-0.7947F,
                 .tang.x   = 0.5878F,.tang.y   = 0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.6429F,.binorm.y = 0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.2764F,.vert.y   =-0.8506F,.vert.z   = 0.4472F,
                 .tex.x    = 0.8182F,.tex.y    = 0.3149F,
                 .norm.x   =-0.1876F,.norm.y   =-0.5774F,.norm.z   = 0.7947F,
                 .tang.x   =-0.9511F,.tang.y   = 0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.2456F,.binorm.y = 0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.0000F,.vert.y   = 0.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.0909F,.tex.y    = 0.0000F,
                 .norm.x   =-0.4911F,.norm.y   =-0.3568F,.norm.z   =-0.7947F,
                 .tang.x   =-0.5878F,.tang.y   = 0.8090F,.tang.z   = 0.0000F,
                 .binorm.x =-0.6429F,.binorm.y =-0.4671F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.0000F,.vert.y   = 0.0000F,.vert.z   =-1.0000F,
                 .tex.x    = 0.4545F,.tex.y    = 0.0000F,
                 .norm.x   = 0.1876F,.norm.y   = 0.5774F,.norm.z   =-0.7947F,
                 .tang.x   = 0.9511F,.tang.y   =-0.3090F,.tang.z   =-0.0000F,
                 .binorm.x = 0.2456F,.binorm.y = 0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.7236F,.vert.y   =-0.5257F,.vert.z   = 0.4472F,
                 .tex.x    = 1.0000F,.tex.y    = 0.3149F,
                 .norm.x   =-0.1876F,.norm.y   =-0.5774F,.norm.z   = 0.7947F,
                 .tang.x   =-0.9511F,.tang.y   = 0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.2456F,.binorm.y = 0.7558F,.binorm.z = 0.6071F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-0.2764F,.vert.y   =-0.8506F,.vert.z   =-0.4472F,
                 .tex.x    = 0.9091F,.tex.y    = 0.1575F,
                 .norm.x   = 0.3035F,.norm.y   =-0.9342F,.norm.z   =-0.1876F,
                 .tang.x   =-0.9511F,.tang.y   =-0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0580F,.binorm.y =-0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.7236F,.vert.y   =-0.5257F,.vert.z   =-0.4472F,
                 .tex.x    = 0.7273F,.tex.y    = 0.1575F,
                 .norm.x   = 0.3035F,.norm.y   =-0.9342F,.norm.z   =-0.1876F,
                 .tang.x   =-0.9511F,.tang.y   =-0.3090F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0580F,.binorm.y =-0.1784F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 0.8944F,.vert.y   = 0.0000F,.vert.z   = 0.4472F,
                 .tex.x    = 0.6364F,.tex.y    = 0.3149F,
                 .norm.x   = 0.7946F,.norm.y   =-0.5774F,.norm.z   = 0.1876F,
                 .tang.x   =-0.5878F,.tang.y   =-0.8090F,.tang.z   =-0.0000F,
                 .binorm.x =-0.1518F,.binorm.y = 0.1103F,.binorm.z = 0.9822F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
        };
        static const size_t nvertices = sizeof(vertices) / sizeof(*vertices);

        static const unsigned indices[] = {
                47, 50, 10,
                12, 30, 44,
                54, 32, 43,
                52, 4, 24,
                55, 46, 48,
                15, 7, 34,
                57, 58, 17,
                27, 38, 1,
                9, 8, 35,
                33, 51, 14,
                16, 59, 20,
                0, 49, 5,
                36, 23, 39,
                13, 40, 18,
                37, 41, 42,
                11, 3, 19,
                56, 53, 21,
                22, 31, 25,
                2, 6, 28,
                26, 45, 29,
        };
        static const size_t nindices = sizeof(indices) / sizeof(*indices);

        struct growingArray vertexList;
        struct growingArray indexList;
        growingArray_init(&vertexList, sizeof(struct vertex), nvertices);
        growingArray_init(&indexList, sizeof(unsigned), nindices);

        for (size_t i=0; i<nvertices; i++) {
                struct vertex *v = growingArray_append(&vertexList);
                *v = vertices[i];
        }
        for (size_t i=0; i<nindices; i++) {
                unsigned *idx = growingArray_append(&indexList);
                *idx = indices[i];
        }

        // Subdivide
        for (unsigned _=0; _<subdivisions; _++) {
                struct growingArray newVertexList;
                struct growingArray newIndexList;
                growingArray_init(&newVertexList,
                                  sizeof(struct vertex), vertexList.length);
                growingArray_init(&newIndexList,
                                  sizeof(unsigned), indexList.length);
                unsigned lastIdx = 0;
                for (size_t i=0; i<indexList.length; i+=3) {
                        unsigned *idxA = growingArray_get(&indexList, i+0);
                        unsigned *idxB = growingArray_get(&indexList, i+1);
                        unsigned *idxC = growingArray_get(&indexList, i+2);
                        
                        struct vertex *old_vA =
                                growingArray_get(&vertexList, *idxA);
                        struct vertex *old_vB =
                                growingArray_get(&vertexList, *idxB);
                        struct vertex *old_vC =
                                growingArray_get(&vertexList, *idxC);

                        struct vertex vA;
                        struct vertex vB;
                        struct vertex vC;
                        struct vertex vAB;
                        struct vertex vAC;
                        struct vertex vBC;
                        
                        // Calculate vertex positions
                        const float half = 0.5F;
                        vA.vert = glms_normalize(old_vA->vert);
                        vB.vert = glms_normalize(old_vB->vert);
                        vC.vert = glms_normalize(old_vC->vert);
                        vAB.vert = glms_normalize(
                                glms_vec3_scale(
                                        glms_vec3_add(
                                                old_vA->vert,
                                                old_vB->vert), half));
                        vAC.vert = glms_normalize(
                                glms_vec3_scale(
                                        glms_vec3_add(
                                                old_vA->vert,
                                                old_vC->vert), half));
                        vBC.vert = glms_normalize(
                                glms_vec3_scale(
                                        glms_vec3_add(
                                                old_vB->vert,
                                                old_vC->vert), half));

                        // Calculate vertex texture coordinates
                        vA.tex = old_vA->tex;
                        vB.tex = old_vB->tex;
                        vC.tex = old_vC->tex;
                        vAB.tex = glms_vec2_scale(
                                glms_vec2_add(
                                        old_vA->tex, old_vB->tex), half);
                        vAC.tex = glms_vec2_scale(
                                glms_vec2_add(
                                        old_vA->tex, old_vC->tex), half);
                        vBC.tex = glms_vec2_scale(
                                glms_vec2_add(
                                        old_vB->tex, old_vC->tex), half);
                        
                        // Calculate vertex normals
                        vA.norm = glms_normalize(
                                glms_cross(
                                        glms_vec3_sub(vAB.vert, vA.vert),
                                        glms_vec3_sub(vAC.vert, vA.vert)));
                        vB.norm = glms_normalize(
                                glms_cross(
                                        glms_vec3_sub(vBC.vert, vB.vert),
                                        glms_vec3_sub(vAB.vert, vB.vert)));
                        vC.norm = glms_normalize(
                                glms_cross(
                                        glms_vec3_sub(vAC.vert, vC.vert),
                                        glms_vec3_sub(vBC.vert, vC.vert)));
                        vAB.norm = glms_normalize(
                                glms_cross(
                                        glms_vec3_sub(vBC.vert, vAB.vert),
                                        glms_vec3_sub(vAC.vert, vAB.vert)));
                        vAC.norm = glms_normalize(
                                glms_cross(
                                        glms_vec3_sub(vAB.vert, vAC.vert),
                                        glms_vec3_sub(vBC.vert, vAC.vert)));
                        vBC.norm = glms_normalize(
                                glms_cross(
                                        glms_vec3_sub(vAC.vert, vBC.vert),
                                        glms_vec3_sub(vAB.vert, vBC.vert)));
                        
                        // TODO: Tangent, binormal
                        
                        // Set bones to 0
                        vA.bones.x = vA.bones.y = vA.bones.z = 0;
                        vA.weights.x = vA.weights.y = vA.weights.z = 0;
                        vB.bones.x = vB.bones.y = vB.bones.z = 0;
                        vB.weights.x = vB.weights.y = vB.weights.z = 0;
                        vC.bones.x = vC.bones.y = vC.bones.z = 0;
                        vC.weights.x = vC.weights.y = vC.weights.z = 0;
                        vAB.bones.x = vAB.bones.y = vAB.bones.z = 0;
                        vAB.weights.x = vAB.weights.y = vAB.weights.z = 0;
                        vAC.bones.x = vAC.bones.y = vAC.bones.z = 0;
                        vAC.weights.x = vAC.weights.y = vAC.weights.z = 0;
                        vBC.bones.x = vBC.bones.y = vBC.bones.z = 0;
                        vBC.weights.x = vBC.weights.y = vBC.weights.z = 0;
                        
                        struct vertex *v;
                        v = growingArray_append(&newVertexList);
                        *v = vA;
                        v = growingArray_append(&newVertexList);
                        *v = vB;
                        v = growingArray_append(&newVertexList);
                        *v = vC;
                        v = growingArray_append(&newVertexList);
                        *v = vAB;
                        v = growingArray_append(&newVertexList);
                        *v = vAC;
                        v = growingArray_append(&newVertexList);
                        *v = vBC;

                        const unsigned first = 0;
                        const unsigned second = 1;
                        const unsigned third = 2;
                        const unsigned fourth = 3;
                        const unsigned fifth = 4;
                        const unsigned sixth = 5;
                        const unsigned all = 6;
                        
                        unsigned *idx;
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + first;
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + fourth;
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + fifth;
                        
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + fourth;
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + second;
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + sixth;
                        
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + fifth;
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + sixth;
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + third;
                        
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + fourth;
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + sixth;
                        idx = growingArray_append(&newIndexList);
                        *idx = lastIdx + fifth;
                        
                        lastIdx += all;
                }
                growingArray_destroy(&vertexList);
                growingArray_destroy(&indexList);
                vertexList = newVertexList;
                indexList = newIndexList;
        }

        geometry_initFromArray(geo, name,
                               vertexList.data, vertexList.length,
                               indexList.data, indexList.length);

        growingArray_destroy(&vertexList);
        growingArray_destroy(&indexList);
}

void geometry_initPlane(struct geometry *geo, const char *name) {
        static const struct vertex vertices[] = {
                {.vert.x   =-1.0000F,.vert.y   = 1.0000F,.vert.z   = 0.0000F,
                 .tex.x    = 0.0000F,.tex.y    = 0.0000F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   = 1.0000F,
                 .tang.x   = 1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   = 1.0000F,.vert.z   = 0.0000F,
                 .tex.x    = 1.0000F,.tex.y    = 0.0000F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   = 1.0000F,
                 .tang.x   = 1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   =-1.0000F,.vert.y   =-1.0000F,.vert.z   = 0.0000F,
                 .tex.x    = 0.0000F,.tex.y    = 1.0000F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   = 1.0000F,
                 .tang.x   = 1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
                {.vert.x   = 1.0000F,.vert.y   =-1.0000F,.vert.z   = 0.0000F,
                 .tex.x    = 1.0000F,.tex.y    = 1.0000F,
                 .norm.x   = 0.0000F,.norm.y   = 0.0000F,.norm.z   = 1.0000F,
                 .tang.x   = 1.0000F,.tang.y   = 0.0000F,.tang.z   = 0.0000F,
                 .binorm.x = 0.0000F,.binorm.y =-1.0000F,.binorm.z = 0.0000F,
                 .bones.x  = 0.0000F,.bones.y  = 0.0000F,.bones.z  = 0.0000F,
                 .weights.x= 0.0000F,.weights.y= 0.0000F,.weights.z= 0.0000F,
                },
        };
        
        static const unsigned indices[] = {
                2, 1, 0,     1, 2, 3
        };
        
        static const size_t nvertices = sizeof(vertices)/sizeof(*vertices);
        static const size_t nindices = sizeof(indices)/sizeof(*indices);
        
        geometry_initFromArray(geo, name,
                               vertices, nvertices, indices, nindices);
}

size_t geometry_initFromFile(struct geometry *const geometry, FILE *const f,
                             const enum componentType type) {
        assert(type == COMPONENT_GEOMETRY);
        
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
        assert(geometry->base.type == COMPONENT_GEOMETRY);
        
        glBindVertexArray(geometry->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->ibo);
        glDrawElements(GL_TRIANGLES, geometry->nindices, GL_UNSIGNED_INT, 0);
}

void geometry_free(struct geometry *const geometry) {
        assert(geometry->base.type == COMPONENT_GEOMETRY);
        
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
