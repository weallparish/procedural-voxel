#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static void printGLError(const char* prefix, GLuint id) {
    // Idk how long errors can get... might need a loop here but hopefully this just doesn't get called.
    char log[1024] = {};
    if (strcmp(prefix, "Shader")) {
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
    }   
    else {
        glGetProgramInfoLog(id, sizeof(log), nullptr, log);
    }
    
    std::fprintf(stderr, "%s link error:\n%s\n", prefix, log);
}

static GLuint compileGLShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);

    // Try to compile the shader.
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);


    if (!ok) {
        printGLError("Shader", s);
    }
    return s;
}

static GLuint createGLProgram(const char* vs, const char* fs) {
    GLuint v = compileGLShader(GL_VERTEX_SHADER, vs);
    GLuint f = compileGLShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);

    // Try to link the program to the shaders
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);

    // Print error if program couldn't link.
    if (!ok) {
        printGLError("Program", p);
    }

    // Program has been linked, we can delete shaders.
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}