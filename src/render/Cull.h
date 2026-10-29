#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>

#include "core/Camera.h"

using Frustum = std::array<glm::vec4, 6>;

glm::vec4 matRow(const glm::mat4& m , int i) {
    return glm::vec4(m[0][i], m[1][i], m[2][i], m[3][i]);
}

Frustum calculatePlanes(glm::mat4x4 vp, Camera cam) {
    glm::vec4 left = matRow(vp, 3) + matRow(vp, 0);
    glm::vec4 right = matRow(vp, 3) - matRow(vp, 0);
    glm::vec4 bottom = matRow(vp, 3) + matRow(vp, 1);
    glm::vec4 top = matRow(vp, 3) - matRow(vp, 1);
    glm::vec4 near = matRow(vp, 3) + matRow(vp, 2);
    glm::vec4 far = matRow(vp, 3) - matRow(vp, 2);

    Frustum planes = {
        left, right, bottom, top, near, far
    };

    for (int i = 0; i < 6; i++) {
        glm::vec4 p = planes[i];
        float len = glm::length(glm::vec3(p));

        planes[i] = p / len;
    }

    glm::vec3 pt = cam.pos - cam.front() * 5.0f;
    for (auto p : planes) {
        float d = glm::dot(glm::vec3(p), pt) + p.w;
    }

    return planes;
}

bool isVisibleToFrustum(Frustum planes, const ChunkPos& pos) {

    glm::vec3 bmin = pos * CHUNK_SIZE;
    glm::vec3 bmax = bmin + glm::vec3{CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE};

    for (int i = 0; i < 6; i++) {
        glm::vec3 normal = glm::vec3(planes[i]);

        glm::vec3 furthestCorner{
            normal.x >= 0 ? bmax.x : bmin.x,
            normal.y >= 0 ? bmax.y : bmin.y,
            normal.z >= 0 ? bmax.z : bmin.z
        };

        if (glm::dot(normal, furthestCorner) + planes[i].w < 0) {
            return false;
        }
    }
    return true;

}