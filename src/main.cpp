// glad must be included before GLFW.
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cmath>
#include <cstdio>
#include <vector>

#include "world/World.h"
#include "world/TerrainGen.h"
#include "world/Chunk.h"
#include "world/Mesher.h"
#include "world/ChunkStreamer.h"
#include "render/ChunkMesh.h"
#include "render/Texture.h"
#include "render/Cull.h"
#include "render/ChunkRenderer.h"
#include "core/Input.h"
#include "core/Camera.h"
#include "core/Debug.h"

// ---------------------------------------------------------------------------
// Shaders (inline for now; move to assets/shaders once you have more than one)
// ---------------------------------------------------------------------------
static const char* kVertSrc = R"(#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 mvp;
out vec3 vColor;
void main() {
    vColor = aColor;
    gl_Position = mvp * vec4(aPos, 1.0);
}
)";

static const char* kFragSrc = R"(#version 450 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

// ---------------------------------------------------------------------------
// Debug grid: lines on y = 0 plus colored axes (X red, Y green, Z blue)
// ---------------------------------------------------------------------------
struct LineVertex {
    glm::vec3 pos;
    glm::vec3 color;
};

static std::vector<LineVertex> buildGrid(int half) {
    std::vector<LineVertex> v;
    for (int i = -half; i <= half; ++i) {
        // Color of every tenth coordinate is different.
        float c = (i % 10 == 0) ? 0.45f : 0.22f;
        glm::vec3 col(c);
        if (i != 0) {
            v.push_back({{(float)-half, 0, (float)i}, col});
            v.push_back({{(float)half, 0, (float)i}, col});
            v.push_back({{(float)i, 0, (float)-half}, col});
            v.push_back({{(float)i, 0, (float)half}, col});
        }
    }
    v.push_back({{(float)-half, 0, 0}, {0.9f, 0.2f, 0.2f}}); // X axis
    v.push_back({{(float)half, 0, 0}, {0.9f, 0.2f, 0.2f}});
    v.push_back({{0, 0, (float)-half}, {0.2f, 0.3f, 0.9f}}); // Z axis
    v.push_back({{0, 0, (float)half}, {0.2f, 0.3f, 0.9f}});
    v.push_back({{0, 0, 0}, {0.2f, 0.9f, 0.2f}});            // Y axis
    v.push_back({{0, 10, 0}, {0.2f, 0.9f, 0.2f}});
    return v;
}

// ---------------------------------------------------------------------------
int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to init GLFW\n");
        return 1;
    }

    // OpenGL 4.5 core. On macOS (max GL 4.1) change to 4.1, add
    // GLFW_OPENGL_FORWARD_COMPAT, and use "#version 410 core" in the shaders.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Procedural Voxel Game", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    int version = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    if (!version) {
        std::fprintf(stderr, "Failed to load OpenGL via glad\n");
        return 1;
    }
    std::printf("OpenGL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

    bool vsync = true;
    glfwSwapInterval(1);

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    // GL state + resources
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    GLuint program = createGLProgram(kVertSrc, kFragSrc);
    GLint mvpLoc = glGetUniformLocation(program, "mvp");

    std::vector<LineVertex> grid = buildGrid(50);
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Put grid data inside the buffer.
    glBufferData(GL_ARRAY_BUFFER, grid.size() * sizeof(LineVertex), grid.data(), GL_STATIC_DRAW);

    // 0th vertex attribute is position with 3 arguments each (x, y, z)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, pos));

    // 1st vertex attribute is color with 3 arguments each (r, g, b)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, color));
    glBindVertexArray(0);

    Camera cam;

    Input input;
    input.setCapture(window, true);

    double lastTime = glfwGetTime();

    // Generate world with a chunk going in the positive and negative direction for each axis.

    BlockRegistry registry;
    World world;
    ChunkStreamer chunkStreamer = ChunkStreamer(12345, registry);
    ChunkRenderer chunkRenderer = ChunkRenderer("assets/textures/atlas.png");
    
    // for (auto& [pos, chunk] : world.chunks) {
    //     MeshData md = meshChunk(world, pos, chunk.get(), registry); // Probably should find a better way to pass this info in. Weird that I pass in the position and the chunk.
    //     if (md.indices.empty()) continue;
    //     meshes[pos].upload(md);
    // }

    int chunksRendered, chunksTotal, chunksMeshed, chunksRemoved = 0;

    // Code to run while the window is still open.
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        glfwPollEvents();

        // Apply user input
        input.applyInput(window, &cam, dt);

        int frameBufferWidth, frameBufferHeight;
        glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);

        // Don't do anything if the application is minimized.
        if (frameBufferWidth == 0 || frameBufferHeight == 0) continue; // minimized

        // ---- ImGui frame ----
        // Tell ImGui that we're going to create windows.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderDebug(chunksRendered, chunksMeshed, chunksTotal, chunksRemoved, &chunkStreamer, &cam, &vsync);
        
        // We're done creating windows, render them.
        ImGui::Render();

        // ---- Scene ----
        glViewport(0, 0, frameBufferWidth, frameBufferHeight);
        glClearColor(0.53f, 0.74f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Rendering

        glm::mat4 proj = glm::perspective(glm::radians(cam.fov), (float)frameBufferWidth / (float)frameBufferHeight, 0.1f, 1000.0f);
        glm::mat4 mvp = proj * cam.view();

        Frustum planes = calculatePlanes(mvp, cam);

        glUseProgram(program);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glBindVertexArray(vao);
        glDrawArrays(GL_LINES, 0, (GLsizei)grid.size());
        glBindVertexArray(0);

        // Chunks
        chunksTotal = world.chunks.size();

        std::vector<ChunkPos> toRemove = chunkStreamer.update(world, cam);
        chunksRemoved = toRemove.size();

        for (auto pos : toRemove) {
            chunkRenderer.drop(pos);
        }

        chunksMeshed = chunkRenderer.update(world, chunkStreamer.chunkRequests, chunkStreamer.radius, registry, chunkStreamer.chunkLimitPerFrame);

        chunksRendered = chunkRenderer.draw(mvp, planes);
        

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Clear all resources we were using.
    chunkRenderer.shutdown();

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}