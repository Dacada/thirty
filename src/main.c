#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdbool.h>
#include <stdio.h>

#include <window.h>
#include <shader.h>

static void processInput(GLFWwindow *window) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
        }
}

static void draw(void) {
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

int main(void) {
        static float vertices[] = {
                0.75f, 0.75f, 0.0f,
                0.25f, 0.0f, 0.0f,
                0.75f, -0.75f, 0.0f,
                
                -0.75f, 0.75f, 0.0f,
                -0.25f, 0.0f, 0.0f,
                -0.75f, -0.75f, 0.0f,
        };
        
        static unsigned int indices[] = {
                0, 1, 2,
                3, 4, 5
        };
        
        GLFWwindow *w = window_init(800, 600, "Hello Triangle - Exercise 1");
        
        unsigned int shader_program = shader_create("simple");
        
        unsigned int vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        unsigned int vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        unsigned int ebo;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glUseProgram(shader_program);
        
        window_setProcessInputCallback(processInput);
        window_setDrawCallback(draw);  
        window_run(w);
}
