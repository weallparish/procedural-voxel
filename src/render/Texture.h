#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

constexpr int TILE_PX = 16;
constexpr int ATLAS_TILES = 4;
constexpr int ATLAS_PX = TILE_PX * ATLAS_TILES;   // 64

std::vector<unsigned char> makeTestAtlas() {
    std::vector<unsigned char> px(ATLAS_PX * ATLAS_PX * 4);

    for (int y = 0; y < ATLAS_PX; ++y) {
        for (int x = 0; x < ATLAS_PX; ++x) {
            int tile = (y / TILE_PX) * ATLAS_TILES + (x / TILE_PX);
            int tx = x % TILE_PX, ty = y % TILE_PX;   // position inside the tile

            unsigned char r = (tile * 20 + 40) % 256;
            unsigned char g = (tile * 30 + 80) % 256;
            unsigned char b = (tile * 40 + 120) % 256;

            bool border = tx == 0 || ty == 0 || tx == TILE_PX - 1 || ty == TILE_PX - 1;
            bool marker = tx < 4 && ty < 4;

            if (border) { r /= 2; g /= 2; b /= 2; }
            if (marker) { r = g = b = 255; }

            unsigned char* p = &px[(y * ATLAS_PX + x) * 4];
            p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
        }
    }
    return px;
}


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
