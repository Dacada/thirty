#include <window.h>
#include <shader.h>
#include <camera.h>
#include <stb_image.h>
#include <cglm/cglm.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

static unsigned int shader;
static unsigned int vao;
static unsigned int textures[2];

static const vec3s cubePositions[] = {
  { .x= 0.0f, .y= 0.0f, .z=  0.0f },
  { .x= 2.0f, .y= 5.0f, .z=-15.0f },
  { .x=-1.5f, .y=-2.2f, .z=- 2.5f },
  { .x=-3.8f, .y=-2.0f, .z=-12.3f },
  { .x= 2.4f, .y=-0.4f, .z=- 3.5f },
  { .x=-1.7f, .y= 3.0f, .z=- 7.5f },
  { .x= 1.3f, .y=-2.0f, .z=- 2.5f },
  { .x= 1.5f, .y= 2.0f, .z=- 2.5f },
  { .x= 1.5f, .y= 0.2f, .z=- 1.5f },
  { .x=-1.3f, .y= 1.0f, .z=- 1.5f }
};
static const vec3s startingCameraPosition = {
        .x=0.0f, .y=0.0f, .z=3.0f
};

static struct camera cam;

static char title[256];
static bool freefly = false;

static void processKeyboardInput(void) {
        float deltaTime = window_timeDelta();

        static unsigned count = 0;
        if (count == 10) {
                int fps = (int)(1.0f/deltaTime);
                snprintf(title, 256, "[%d] Boxes (%s)", fps,
                         freefly ? "Freefly camera" : "FPS camera");
                window_updateTitle();
                count = 0;
        } else {
                count++;
        }
        
        if (window_keyPressed(GLFW_KEY_W)) {
                camera_move(&cam, camera_FORWARD, deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_S)) {
                camera_move(&cam, camera_BACKWARD, deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_A)) {
                camera_move(&cam, camera_LEFT, deltaTime, freefly);
        }
        if (window_keyPressed(GLFW_KEY_D)) {
                camera_move(&cam, camera_RIGHT, deltaTime, freefly);
        }
}

static void processKeyboardEvent(const int key, const int action,
                                 const int modifiers) {
        (void)modifiers;
        if (action == GLFW_PRESS) {
                if (key == GLFW_KEY_ESCAPE) {
                        window_close();
                } else if (key == GLFW_KEY_F) {
                        freefly = !freefly;
                }
        }
}

static void processMousePosition(const double xpos, const double ypos) {
        static bool first = true;
        static vec2s last;

        vec2s pos = {
                .x=(float)xpos,
                .y=(float)ypos
        };

        if (first) {
                last = pos;
                first = false;
        } else {
                vec2s offset = {
                        .x=pos.x - last.x,
                        .y=last.y - pos.y
                };
                last = pos;
                camera_look(&cam, offset.x, offset.y, true);
        }
}

static void processMouseScroll(const double offset) {
        camera_zoom(&cam, (float)offset);
}

static void draw(void) {
        float fov = glm_rad(cam.zoom_level);
        float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
        mat4s projection = glms_perspective(fov, aspect, 0.1f, 100.0f);
        mat4s view = camera_viewMatrix(&cam);

        shader_use(shader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures[1]);

        shader_setMat4(shader, "view", view);
        shader_setMat4(shader, "projection", projection);

        glBindVertexArray(vao);

        for (int i=0; i<10; i++) {
                mat4s model = GLMS_MAT4_IDENTITY_INIT;
                model = glms_translate(model, cubePositions[i]);

                static const vec3s rot_axis = (vec3s){
                        .x=1.0f,
                        .y=0.3f,
                        .z=0.5f
                };
                
                float angle = (float)i * 20.0f;
                model = glms_rotate(model, angle, rot_axis);
                shader_setMat4(shader, "model", model);

                glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
                fprintf(stderr, "OpenGL Error: %u", error);
        }
}

int main(void) {
        float vertices[] = {
                -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
                 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
                 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
                
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
                 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
                -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                
                -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                
                 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
                 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
                 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
                -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
                -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
                
                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
                 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
                 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
                -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
                -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
        };
        
        camera_init(&cam, &startingCameraPosition, NULL, NULL, NULL);
        cam.look_sensitivity = 0.01f;

        window_init(SCREEN_WIDTH, SCREEN_HEIGHT);
        window_title = title;
        shader = shader_new("mix_two_textures", "simple");

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        unsigned int vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
                     vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              5*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              5*sizeof(float), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);

        glGenTextures(2, textures);
        stbi_set_flip_vertically_on_load(true);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nrChannels;
        unsigned char *data = stbi_load(ASSETSPATH "/textures/container.jpg",
                                        &width, &height, &nrChannels, 0);
        if (data == NULL) {
                fprintf(stderr, "Failed to load texture 1");
                return 1;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                     0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
  
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures[1]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  
        data = stbi_load(ASSETSPATH "/textures/awesomeface.png",
                         &width, &height, &nrChannels, 0);
        if (data == NULL) {
                fprintf(stderr, "Failed to load texture 2");
                return 1;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);

        shader_use(shader);
        shader_setInt(shader, "ourTexture1", 0);
        shader_setInt(shader, "ourTexture2", 1);

        window_onKeyboardInput = processKeyboardInput;
        window_onKeyboardEvent = processKeyboardEvent;
        window_onMousePosition = processMousePosition;
        window_onMouseScroll = processMouseScroll;
        window_onDraw = draw;
        
        window_run();
        return 0;
}
