#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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