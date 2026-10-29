#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

constexpr int TILE_PX = 16;
constexpr int ATLAS_WIDTH = 10;
constexpr int ATLAS_HEIGHT = 16;
constexpr int ATLAS_PX = TILE_PX * ATLAS_WIDTH * ATLAS_HEIGHT;   // 64

GLuint uploadAtlas(const unsigned char* data, int w, int h) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(
        GL_TEXTURE_2D, 
        0, 
        GL_RGBA8, 
        w, 
        h, 
        0, 
        GL_RGBA, 
        GL_UNSIGNED_BYTE, 
        data
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    
    return tex;
}

GLuint loadTexture(const char* path) {
    int w, h, channels;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(path, &w, &h, &channels, 4);
    if (!data) {
        std::fprintf(stderr, "Failed to load %s\n", path);
        return 0;
    }

    GLuint tex = uploadAtlas(data, w, h);
    stbi_image_free(data);

    return tex;
}
