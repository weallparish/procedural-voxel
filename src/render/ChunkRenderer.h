#pragma once

#include <unordered_map>
#include <unordered_set>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "world/World.h"
#include "Texture.h"

static const char* vertSrc = R"(#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in float aShade;

uniform mat4 mvp;
uniform vec3 uChunkOffset;

out vec2 vUV;
out float vShade;
void main() {
    vUV = aUV;
    vShade = aShade;
    gl_Position = mvp * vec4(aPos + uChunkOffset, 1.0);
}
)";

static const char* fragSrc = R"(#version 450 core
in vec2 vUV;
in float vShade;

uniform sampler2D uAtlas;

out vec4 FragColor;
void main() {
    FragColor = vec4((texture(uAtlas, vUV)) * vShade);
}
)";

static void printGLError(const char* prefix, GLuint id) {
    // Idk how long errors can get... might need a loop here but hopefully this just doesn't get called.
    char log[1024] = {};
    if (prefix == "Shader") {
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

struct ChunkRenderer {
    std::unordered_map<ChunkPos, ChunkMesh, ChunkPosHash> meshes;

    GLuint chunkProgram;
    GLint chunkMvpLoc;
    GLint offsetLoc;

    GLuint atlas;

    std::unordered_set<ChunkPos, ChunkPosHash> processedChunks;

    glm::vec3 neighbors[6] = {
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
        {0, 0, 1}, {0, 0, -1}
    };

    // Make sure this isn't copied.
    ChunkRenderer(ChunkRenderer const &) = delete;

    ChunkRenderer(const char* atlasPath) {
        chunkProgram = createGLProgram(vertSrc, fragSrc);
        chunkMvpLoc = glGetUniformLocation(chunkProgram, "mvp");
        offsetLoc = glGetUniformLocation(chunkProgram, "uChunkOffset");
        glUniform1i(glGetUniformLocation(chunkProgram, "uAtlas"), 0);

        atlas = loadTexture(atlasPath);
    }

    bool chunkProcessed(ChunkPos pos) {
        return processedChunks.contains(pos);
    }

    bool neighborExists(const World& world, WantedChunk chunk, glm::vec3 offset,  int meshRadius) {
        ChunkPos neighborPos = {chunk.pos.x + offset.x, chunk.pos.y + offset.y, chunk.pos.z + offset.z};
        return world.chunks.contains(neighborPos);
    }

    int update(const World& world, std::vector<WantedChunk>& chunkRequests, int meshRadius, BlockRegistry& reg, int meshLimit) {
        int budget = meshLimit;
        for (auto request : chunkRequests) {
            if (budget == 0) break;

            if (request.distance > meshRadius * meshRadius) continue;

            if (chunkProcessed(request.pos)) continue;

            Chunk* c = world.getChunk(request.pos);
            if (!c) continue;

            bool neighborMissing = false;
            for (auto offset : neighbors) {
                if (!neighborExists(world, request, offset, meshRadius)) {
                    neighborMissing = true;
                    break;
                }
            }
            if (neighborMissing) continue;

            MeshData md = meshChunk(world, request.pos, c, reg);
            budget--;
            processedChunks.insert({request.pos});

            if (md.indices.empty()) continue;
            meshes[request.pos].upload(md);
        }

        return processedChunks.size();
    }

    void drop(ChunkPos pos) {
        auto meshIt = meshes.find(pos);
        processedChunks.erase(pos);
        if (meshIt != meshes.end()) {
            meshIt->second.destroy();
            meshes.erase(meshIt);
        }
    }

    int draw(glm::mat4 mvp, Frustum frustum) {
        glUseProgram(chunkProgram);
        glUniformMatrix4fv(chunkMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, atlas);

        int renderedChunks = 0;

        for (auto& [pos, mesh] : meshes) {
            if (!isVisibleToFrustum(frustum, pos)) {
                continue;
            };
            glm::vec3 offset(pos.x * CHUNK_SIZE, pos.y * CHUNK_SIZE, pos.z * CHUNK_SIZE);
            glUniform3fv(offsetLoc, 1, glm::value_ptr(offset));
            mesh.draw();
            renderedChunks++;
        }

        return renderedChunks;
    }

    void shutdown() {
        for (auto& [pos, mesh] : meshes) mesh.destroy();
        meshes.clear();

        glDeleteTextures(0, &atlas);
        glDeleteProgram(chunkProgram);
    }
};