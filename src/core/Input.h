#pragma once

#include <glad/gl.h> // I do not know why there is an error here... it compiles.
#include <GLFW/glfw3.h>

#include "Camera.h"

struct Input {
    bool captured = true;
    bool firstMouse = true;
    bool escWasDown = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;

    void applyInput(GLFWwindow* window, Camera* cam, float dt) {
        // Esc toggles mouse capture (so you can use the ImGui widgets)
        bool escDown = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (escDown && !escWasDown) setCapture(window, !captured);
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
            cam->yaw += dx * cam->sensitivity;
            cam->pitch -= dy * cam->sensitivity;
            cam->pitch = glm::clamp(cam->pitch, -89.0f, 89.0f);

            // Movement
            float v = cam->speed * dt;
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) v *= 3.0f;
            glm::vec3 f = cam->front(), r = cam->right();
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cam->pos += f * v;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cam->pos -= f * v;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cam->pos += r * v;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cam->pos -= r * v;
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cam->pos.y += v;
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) cam->pos.y -= v;
        }
    }

    void setCapture(GLFWwindow* window, bool on) {
        captured = on;
        firstMouse = true;
        glfwSetInputMode(window, GLFW_CURSOR, on ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (on && glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    };
};
