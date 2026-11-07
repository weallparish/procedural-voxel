#pragma once

#include <unordered_map>
#include <unordered_set>

static const char* outlineVertSrc = R"(#version 450 core
layout(location = 0) in vec3 aPos;

uniform mat4 mvp;
uniform vec3 uBlockOffset;

out vec3 vPos;

void main() {
    vPos = aPos;
    gl_Position = mvp * vec4(aPos + uBlockOffset, 1.0);
}
)";

static const char* outlineFragSrc = R"(#version 450 core
in vec3 vPos;

out vec4 FragColor;
void main() {
    FragColor = vec4(vPos, 1.0);
}
)";

static const std::vector<float> outlineVertices = {
    -0.002f, -0.002f, -0.002f,
    1.002f, -0.002f, -0.002f,
    -0.002f, 1.002f, -0.002f,
    -0.002f, -0.002f, 1.002f,
    1.002f, 1.002f, -0.002f,
    1.002f, -0.002f, 1.002f,
    -0.002f, 1.002f, 1.002f,
    1.002f, 1.002f, 1.002f
};

static const std::vector<uint32_t> outlineIndices = {
    // bottom (y = 0)
    0, 1,  1, 5,  5, 3,  3, 0,
    // top (y = 1)
    2, 4,  4, 7,  7, 6,  6, 2,
    // vertical edges
    0, 2,  1, 4,  5, 7,  3, 6
};

struct BlockOutlineRenderer {
    GLuint outlineProgram;
    GLint outlineMvpLoc;
    GLint offsetLoc;

    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;

    // Make sure this isn't copied.
    BlockOutlineRenderer(BlockOutlineRenderer const &) = delete;

    BlockOutlineRenderer() {
        outlineProgram = createGLProgram(outlineVertSrc, outlineFragSrc);
        glUseProgram(outlineProgram);

        outlineMvpLoc = glGetUniformLocation(outlineProgram, "mvp");
        offsetLoc = glGetUniformLocation(outlineProgram, "uBlockOffset");

        // Create arrays/buffers.
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, outlineVertices.size() * sizeof(float), outlineVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, outlineIndices.size() * sizeof(uint32_t), outlineIndices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

        glBindVertexArray(0);
        indexCount = (GLsizei)outlineIndices.size();
    }

    void draw(glm::mat4 mvp, glm::vec3 pos) {
        glUseProgram(outlineProgram);
        glUniformMatrix4fv(outlineMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        glm::vec3 offset(pos.x, pos.y, pos.z);
        glUniform3fv(offsetLoc, 1, glm::value_ptr(offset));

        glBindVertexArray(vao);
        glDrawElements(GL_LINES, indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    void shutdown() {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
        vao = vbo = ebo = 0;

        glDeleteProgram(outlineProgram);
    }
};