// glad must be included before GLFW.
#include <glad/gl.h> // I do not know why there is an error here... it compiles.
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

#include "world/Chunk.h"
#include "world/Mesher.h"
#include "render/ChunkMesh.h"
#include "render/Texture.h"

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

static const char* lVertSrc = R"(#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in float aShade;

uniform mat4 mvp;

out vec2 vUV;
out float vShade;
void main() {
    vUV = aUV;
    vShade = aShade;
    gl_Position = mvp * vec4(aPos, 1.0);
}
)";

static const char* lFragSrc = R"(#version 450 core
in vec2 vUV;
in float vShade;

uniform sampler2D uAtlas;

out vec4 FragColor;
void main() {
    FragColor = vec4(texture(uAtlas, vUV) * vShade);
}
)";

static void printGLError(const char* prefix, GLuint id) {
    // Idk how long errors can get... might need a loop here but hopefully this just doesn't get called.
    char log[1024];
    glGetProgramInfoLog(id, sizeof(log), nullptr, log);
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

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------
struct Camera {
    glm::vec3 pos{0.0f, 3.0f, 8.0f};
    float yaw = -90.0f;   // degrees; -90 looks down -Z
    float pitch = 0.0f;   // degrees
    float fov = 70.0f;
    float speed = 8.0f;   // blocks per second
    float sensitivity = 0.1f;

    // Get vector pointing out the front of the character.
    glm::vec3 front() const {
        float y = glm::radians(yaw), p = glm::radians(pitch);
        return glm::normalize(glm::vec3(std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)));
    }

    // Get vector pointing out the right of the character.
    glm::vec3 right() const { 
        // Right vector is orthogonal to front and up.
        return glm::normalize(glm::cross(front(), glm::vec3(0, 1, 0))); 
    }

    // Get view projection matrix of the camera.
    glm::mat4 view() const { 
        // We are looking from our position towards the front vector.
        return glm::lookAt(pos, pos + front(), glm::vec3(0, 1, 0)); 
    }
};

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

    GLuint chunkProgram = createGLProgram(lVertSrc, lFragSrc);
    GLint chunkMvpLoc = glGetUniformLocation(chunkProgram, "mvp");

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

    auto atlasPixels = makeTestAtlas();
    GLuint atlasTex = uploadAtlas(atlasPixels.data(), ATLAS_PX, ATLAS_PX);

    Camera cam;
    bool captured = true;
    bool firstMouse = true;
    bool escWasDown = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    double lastTime = glfwGetTime();

    auto setCapture = [&](bool on) {
        captured = on;
        firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, on ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (on && glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    };
    setCapture(true);

    Chunk c;
    MeshData mesh;
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            for (int y = 0; y < 16; y++) {
                c.set(x, y, z, 1);
            }
        }
    }

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            for (int y = 0; y < 16; y++) {
                if (!isAir(c, x, y, z)) {
                    for (int i = 0; i < 6; i++) {
                        if (isAir(c, x + FACES[i].dx, y + FACES[i].dy, z + FACES[i].dz)) {
                            addFace(mesh, x, y, z, i, y);
                        }
                    }
                }
            }
        }
    }

    ChunkMesh chunkMesh;
    chunkMesh.upload(mesh);

    // Code to run while the window is still open.
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        glfwPollEvents();

        // Esc toggles mouse capture (so you can use the ImGui widgets)
        bool escDown = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (escDown && !escWasDown) setCapture(!captured);
        escWasDown = escDown;

        if (captured) {
            // Mouse look
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            if (firstMouse) {
                lastMouseX = mx;
                lastMouseY = my;
                firstMouse = false;
            }
            float dx = (float)(mx - lastMouseX);
            float dy = (float)(my - lastMouseY);
            lastMouseX = mx;
            lastMouseY = my;
            cam.yaw += dx * cam.sensitivity;
            cam.pitch -= dy * cam.sensitivity;
            cam.pitch = glm::clamp(cam.pitch, -89.0f, 89.0f);

            // Movement
            float v = cam.speed * dt;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) v *= 3.0f;
            glm::vec3 f = cam.front(), r = cam.right();
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cam.pos += f * v;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cam.pos -= f * v;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cam.pos += r * v;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cam.pos -= r * v;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cam.pos.y += v;
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) cam.pos.y -= v;
        }

        int frameBufferWidth, frameBufferHeight;
        glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);

        // Don't do anything if the application is minimized.
        if (frameBufferWidth == 0 || frameBufferHeight == 0) continue; // minimized

        // ---- ImGui frame ----
        // Tell ImGui that we're going to create windows.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Debug Window
        ImGui::Begin("Debug");
        ImGui::Text("%.1f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("Pos: %.1f, %.1f, %.1f", cam.pos.x, cam.pos.y, cam.pos.z);
        ImGui::SliderFloat("FOV", &cam.fov, 40.0f, 110.0f);
        ImGui::SliderFloat("Speed", &cam.speed, 1.0f, 50.0f);

        // This is called when the checkbox is selected.
        if (ImGui::Checkbox("VSync", &vsync)) 
        {
            glfwSwapInterval(vsync ? 1 : 0);
        }
            
        ImGui::Separator();
        ImGui::TextDisabled("WASD move, Space/Ctrl up/down, Shift sprint");
        ImGui::TextDisabled("Esc: toggle mouse capture");
        ImGui::End();

        // We're done creating windows, render them.
        ImGui::Render();

        // ---- Scene ----
        glViewport(0, 0, frameBufferWidth, frameBufferHeight);
        glClearColor(0.53f, 0.74f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(cam.fov), (float)frameBufferWidth / (float)frameBufferHeight, 0.1f, 1000.0f);
        glm::mat4 mvp = proj * cam.view();

        glUseProgram(program);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glBindVertexArray(vao);
        glDrawArrays(GL_LINES, 0, (GLsizei)grid.size());
        glBindVertexArray(0);

        glUseProgram(chunkProgram);
        glUniformMatrix4fv(chunkMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(glGetUniformLocation(chunkProgram, "uAtlas"), 0);
        glBindTexture(GL_TEXTURE_2D, atlasTex);
        
        chunkMesh.draw();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Clear all resources we were using.
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