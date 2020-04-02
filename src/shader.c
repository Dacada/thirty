#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <shader.h>

__attribute__((noreturn)) static void bail(const char *const msg) {
        if (msg != NULL) perror(msg);
        exit(EXIT_FAILURE);
}

static char *read_all_text(const char *const filename) {
        FILE *f;
        long size;
        size_t usize;
        char *buff;
        
        if ((f = fopen(filename, "rb")) == NULL) bail(filename);
        
        if (fseek(f, 0L, SEEK_END) == -1) bail("fseek");
        if ((size = ftell(f)) < 0) bail("ftell");
        usize = (size_t)size;
        if (fseek(f, 0L, SEEK_SET) == -1) bail("fseek");
        
        if ((buff = malloc((usize+1) * sizeof(char))) == NULL) bail("malloc");
        if (fread(buff, sizeof(char), usize, f) != usize && ferror(f) != 0) bail("fread");
        if (fclose(f) != 0) bail("fclose");
        
        buff[size] = '\0';
        return buff;
}

unsigned int shader_create(const char *const filename) {
        int success, length;
        char *info_log;
        static char full_filename[2048];
        
        full_filename[0] = '\0';
        strcat(full_filename, ASSETSPATH);
        strcat(full_filename, "shaders/");
        strcat(full_filename, filename);
        strcat(full_filename, ".vert");
        
        char *vertex_shader_src = read_all_text(full_filename);
        unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        // http://c-faq.com/ansi/constmismatch.html
        glShaderSource(vertex_shader, 1, (const char *const *)&vertex_shader_src, NULL);
        glCompileShader(vertex_shader);
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (success == 0) {
                glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &length);
                info_log = malloc((size_t)length*sizeof(char));
                glGetShaderInfoLog(vertex_shader, length, NULL, info_log);
                fprintf(stderr, "Error compiling vertex shader. Info log follows.\n%s\n", info_log);
                free(info_log);
                bail(NULL);
        }
        free(vertex_shader_src);
        
        full_filename[0] = '\0';
        strcat(full_filename, ASSETSPATH);
        strcat(full_filename, "shaders/");
        strcat(full_filename, filename);
        strcat(full_filename, ".frag");
        
        char *fragment_shader_src = read_all_text(full_filename);
        unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        // http://c-faq.com/ansi/constmismatch.html
        glShaderSource(fragment_shader, 1, (const char *const *)&fragment_shader_src, NULL);
        glCompileShader(fragment_shader);
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (success == 0) {
                glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &length);
                info_log = malloc((size_t)length*sizeof(char));
                glGetShaderInfoLog(fragment_shader, length, NULL, info_log);
                fprintf(stderr, "Error compiling fragment shader. Info log follows.\n%s\n", info_log);
                free(info_log);
                bail(NULL);
        }
        free(fragment_shader_src);
        
        unsigned int shader_program = glCreateProgram();
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (success == 0) {
                glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &length);
                info_log = malloc((size_t)length*sizeof(char));
                glGetProgramInfoLog(shader_program, length, NULL, info_log);
                fprintf(stderr, "Error linking shader program. Info log follows.\n%s\n", info_log);
                free(info_log);
                bail(NULL);
        }
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        
        return shader_program;
}
