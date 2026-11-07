#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <optional>

#include "utils/VoxelMath.h"
#include "world/World.h"
#include "world/BlockRegistry.h"


constexpr int MAX_STEPS = 16;

// Based on Amantides and Woo's fast voxel traversal algorithm.
std::optional<glm::vec3> castToNearest(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, const World& world, const BlockRegistry& reg) {
    // Variable initialization.
    glm::vec3 step = signOfDir(rayDirection);
    glm::vec3 tDelta = abs(glm::vec3(1) / rayDirection);
    glm::vec3 pos = floor(rayOrigin);
    glm::vec3 tMax = (pos - rayOrigin + max(step, glm::vec3(0))) / rayDirection;

    // Traversal loop
    for (int steps = 0; steps < MAX_STEPS; ++steps) {
        // Get block at current position.
        BlockDef block = reg.get(world.getBlock(pos.x, pos.y, pos.z));

        if (block.solid) {
            return pos;
        }

        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                pos.x += step.x;
                tMax.x += tDelta.x;
            }
            else {
                pos.z += step.z;
                tMax.z += tDelta.z;
            }
        }
        else {
            if (tMax.y < tMax.z) {
                pos.y += step.y;
                tMax.y += tDelta.y;
            }
            else {
                pos.z += step.z;
                tMax.z += tDelta.z;
            }
        }
    }

    return {};
}